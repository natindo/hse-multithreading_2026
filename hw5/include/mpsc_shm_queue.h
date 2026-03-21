#pragma once

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace hw5 {

struct Message {
    std::uint32_t type = 0;
    std::vector<std::uint8_t> payload;
};

class SharedMemoryMpscQueue {
public:
    static constexpr std::uint32_t kMagic = 0x51554555;  // "QUEU"
    static constexpr std::uint32_t kVersion = 1;

    SharedMemoryMpscQueue() = default;

    SharedMemoryMpscQueue(const SharedMemoryMpscQueue&) = delete;
    SharedMemoryMpscQueue& operator=(const SharedMemoryMpscQueue&) = delete;

    SharedMemoryMpscQueue(SharedMemoryMpscQueue&& other) noexcept {
        MoveFrom(std::move(other));
    }

    SharedMemoryMpscQueue& operator=(SharedMemoryMpscQueue&& other) noexcept {
        if (this != &other) {
            Close();
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~SharedMemoryMpscQueue() {
        Close();
    }

    static SharedMemoryMpscQueue Create(const std::string& name, std::size_t queue_capacity_bytes) {
        if (queue_capacity_bytes < 1024) {
            throw std::runtime_error("queue_capacity_bytes must be >= 1024");
        }

        const int fd = shm_open(name.c_str(), O_CREAT | O_RDWR, 0666);
        if (fd < 0) {
            throw std::runtime_error("shm_open(O_CREAT) failed");
        }

        SharedMemoryMpscQueue queue;
        queue.name_ = name;
        queue.fd_ = fd;
        queue.total_size_ = sizeof(ControlBlock) + queue_capacity_bytes;

        if (ftruncate(fd, static_cast<off_t>(queue.total_size_)) != 0) {
            queue.Close();
            throw std::runtime_error("ftruncate failed");
        }

        queue.Map();
        queue.InitializeControl(queue_capacity_bytes);
        return queue;
    }

    static SharedMemoryMpscQueue Open(const std::string& name) {
        const int fd = shm_open(name.c_str(), O_RDWR, 0666);
        if (fd < 0) {
            throw std::runtime_error("shm_open failed");
        }

        struct stat st {
        };
        if (fstat(fd, &st) != 0) {
            close(fd);
            throw std::runtime_error("fstat failed");
        }

        if (st.st_size < static_cast<off_t>(sizeof(ControlBlock) + 1024)) {
            close(fd);
            throw std::runtime_error("shared segment is too small");
        }

        SharedMemoryMpscQueue queue;
        queue.name_ = name;
        queue.fd_ = fd;
        queue.total_size_ = static_cast<std::size_t>(st.st_size);
        queue.Map();
        queue.ValidateControl();
        return queue;
    }

    void Close() {
        if (mapped_ != nullptr) {
            munmap(mapped_, total_size_);
            mapped_ = nullptr;
        }
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

    void Unlink() const {
        if (!name_.empty()) {
            shm_unlink(name_.c_str());
        }
    }

    bool Push(std::uint32_t type, std::string_view bytes) {
        const std::uint32_t payload_size = static_cast<std::uint32_t>(bytes.size());
        const std::uint64_t record_size = sizeof(MessageHeader) + payload_size;

        std::uint64_t reservation = 0;
        while (true) {
            std::uint64_t reserve_pos = control_->reserve_pos.load(std::memory_order_relaxed);
            const std::uint64_t consumed_pos = control_->consumed_pos.load(std::memory_order_acquire);
            if ((reserve_pos - consumed_pos) + record_size > control_->capacity_bytes) {
                return false;
            }
            const std::uint64_t next = reserve_pos + record_size;
            if (control_->reserve_pos.compare_exchange_weak(
                    reserve_pos,
                    next,
                    std::memory_order_acq_rel,
                    std::memory_order_relaxed)) {
                reservation = reserve_pos;
                break;
            }
        }

        MessageHeader header{type, payload_size};
        WriteBytes(reservation, reinterpret_cast<const std::uint8_t*>(&header), sizeof(header));
        WriteBytes(reservation + sizeof(header), reinterpret_cast<const std::uint8_t*>(bytes.data()), payload_size);

        while (control_->published_pos.load(std::memory_order_acquire) != reservation) {
            std::this_thread::yield();
        }

        control_->published_pos.store(reservation + record_size, std::memory_order_release);
        return true;
    }

    bool TryPop(Message& out) {
        const std::uint64_t consumed = control_->consumed_pos.load(std::memory_order_relaxed);
        const std::uint64_t published = control_->published_pos.load(std::memory_order_acquire);
        if (consumed == published) {
            return false;
        }

        MessageHeader header{};
        ReadBytes(consumed, reinterpret_cast<std::uint8_t*>(&header), sizeof(header));

        out.type = header.type;
        out.payload.resize(header.payload_size);
        ReadBytes(consumed + sizeof(header), out.payload.data(), header.payload_size);

        control_->consumed_pos.store(
            consumed + sizeof(MessageHeader) + header.payload_size,
            std::memory_order_release);
        return true;
    }

    bool TryPopByType(std::uint32_t wanted_type, Message& out) {
        Message msg;
        while (TryPop(msg)) {
            if (msg.type == wanted_type) {
                out = std::move(msg);
                return true;
            }
        }
        return false;
    }

private:
    struct MessageHeader {
        std::uint32_t type;
        std::uint32_t payload_size;
    };

    struct alignas(64) ControlBlock {
        std::uint32_t magic;
        std::uint32_t version;
        std::uint64_t capacity_bytes;
        std::atomic<std::uint64_t> reserve_pos;
        std::atomic<std::uint64_t> published_pos;
        std::atomic<std::uint64_t> consumed_pos;
    };

    void MoveFrom(SharedMemoryMpscQueue&& other) {
        name_ = std::move(other.name_);
        fd_ = other.fd_;
        total_size_ = other.total_size_;
        mapped_ = other.mapped_;
        control_ = other.control_;
        buffer_ = other.buffer_;

        other.fd_ = -1;
        other.total_size_ = 0;
        other.mapped_ = nullptr;
        other.control_ = nullptr;
        other.buffer_ = nullptr;
    }

    void Map() {
        mapped_ = mmap(nullptr, total_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
        if (mapped_ == MAP_FAILED) {
            mapped_ = nullptr;
            throw std::runtime_error("mmap failed");
        }
        control_ = static_cast<ControlBlock*>(mapped_);
        buffer_ = reinterpret_cast<std::uint8_t*>(mapped_) + sizeof(ControlBlock);
    }

    void InitializeControl(std::size_t capacity_bytes) {
        control_->magic = kMagic;
        control_->version = kVersion;
        control_->capacity_bytes = capacity_bytes;
        control_->reserve_pos.store(0, std::memory_order_relaxed);
        control_->published_pos.store(0, std::memory_order_relaxed);
        control_->consumed_pos.store(0, std::memory_order_relaxed);
    }

    void ValidateControl() const {
        if (control_->magic != kMagic || control_->version != kVersion) {
            throw std::runtime_error("protocol mismatch");
        }
        if (control_->capacity_bytes + sizeof(ControlBlock) > total_size_) {
            throw std::runtime_error("corrupted control block");
        }
    }

    void WriteBytes(std::uint64_t absolute_pos, const std::uint8_t* src, std::size_t len) {
        if (len == 0) {
            return;
        }

        const std::size_t cap = static_cast<std::size_t>(control_->capacity_bytes);
        std::size_t off = static_cast<std::size_t>(absolute_pos % cap);
        const std::size_t first = std::min(len, cap - off);

        std::memcpy(buffer_ + off, src, first);
        if (len > first) {
            std::memcpy(buffer_, src + first, len - first);
        }
    }

    void ReadBytes(std::uint64_t absolute_pos, std::uint8_t* dst, std::size_t len) const {
        if (len == 0) {
            return;
        }

        const std::size_t cap = static_cast<std::size_t>(control_->capacity_bytes);
        std::size_t off = static_cast<std::size_t>(absolute_pos % cap);
        const std::size_t first = std::min(len, cap - off);

        std::memcpy(dst, buffer_ + off, first);
        if (len > first) {
            std::memcpy(dst + first, buffer_, len - first);
        }
    }

    std::string name_;
    int fd_ = -1;
    std::size_t total_size_ = 0;
    void* mapped_ = nullptr;
    ControlBlock* control_ = nullptr;
    std::uint8_t* buffer_ = nullptr;
};

class ProducerNode {
public:
    ProducerNode(const std::string& shm_name, std::size_t queue_capacity_bytes)
        : queue_(SharedMemoryMpscQueue::Create(shm_name, queue_capacity_bytes)) {
    }

    bool Send(std::uint32_t type, std::string_view payload) {
        return queue_.Push(type, payload);
    }

    void CleanupSharedMemory() const {
        queue_.Unlink();
    }

private:
    SharedMemoryMpscQueue queue_;
};

class ConsumerNode {
public:
    explicit ConsumerNode(const std::string& shm_name)
        : queue_(SharedMemoryMpscQueue::Open(shm_name)) {
    }

    bool TryRecvByType(std::uint32_t type, Message& out) {
        return queue_.TryPopByType(type, out);
    }

private:
    SharedMemoryMpscQueue queue_;
};

}  // namespace hw5
