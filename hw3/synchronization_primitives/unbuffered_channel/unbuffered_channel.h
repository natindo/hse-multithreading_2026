#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <stdexcept>

template <class T>
class UnbufferedChannel {
public:
    void Send(const T& value) {
        std::unique_lock<std::mutex> lock(mutex_);

        sender_turn_cv_.wait(lock, [this]() { return closed_ || !send_in_progress_; });
        if (closed_) {
            throw std::runtime_error("channel is closed");
        }

        send_in_progress_ = true;
        send_aborted_ = false;
        slot_ = value;
        has_value_ = true;
        receiver_cv_.notify_one();

        sender_done_cv_.wait(lock, [this]() { return send_aborted_ || !has_value_; });

        send_in_progress_ = false;
        sender_turn_cv_.notify_one();

        if (send_aborted_) {
            send_aborted_ = false;
            throw std::runtime_error("channel is closed");
        }
    }

    std::optional<T> Recv() {
        std::unique_lock<std::mutex> lock(mutex_);

        receiver_cv_.wait(lock, [this]() { return closed_ || has_value_; });
        if (!has_value_) {
            return std::nullopt;
        }

        std::optional<T> result = std::move(slot_);
        slot_.reset();
        has_value_ = false;

        sender_done_cv_.notify_one();
        return result;
    }

    void Close() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return;
        }

        closed_ = true;

        if (send_in_progress_ && has_value_) {
            send_aborted_ = true;
            has_value_ = false;
            slot_.reset();
        }

        sender_turn_cv_.notify_all();
        sender_done_cv_.notify_all();
        receiver_cv_.notify_all();
    }

private:
    std::mutex mutex_;
    std::condition_variable sender_turn_cv_;
    std::condition_variable sender_done_cv_;
    std::condition_variable receiver_cv_;

    bool closed_ = false;
    bool has_value_ = false;
    bool send_in_progress_ = false;
    bool send_aborted_ = false;
    std::optional<T> slot_;
};
