#pragma once

#include <atomic>
#include <cstdint>
#include <climits>
#include <cerrno>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

class FutexConditionVariable {
public:
    FutexConditionVariable() = default;
    FutexConditionVariable(const FutexConditionVariable&) = delete;
    FutexConditionVariable& operator=(const FutexConditionVariable&) = delete;

    template <class Lock>
    void Wait(Lock& lock) {
        const std::uint32_t observed = seq_.load(std::memory_order_relaxed);
        lock.unlock();
        FutexWait(observed);
        lock.lock();
    }

    template <class Lock, class Predicate>
    void Wait(Lock& lock, Predicate predicate) {
        while (!predicate()) {
            Wait(lock);
        }
    }

    void NotifyOne() {
        seq_.fetch_add(1, std::memory_order_release);
        FutexWake(1);
    }

    void NotifyAll() {
        seq_.fetch_add(1, std::memory_order_release);
        FutexWake(INT_MAX);
    }

private:
    void FutexWait(std::uint32_t expected) {
        for (;;) {
            const long rc = syscall(
                SYS_futex,
                reinterpret_cast<int*>(&seq_),
                FUTEX_WAIT_PRIVATE,
                static_cast<int>(expected),
                nullptr,
                nullptr,
                0);
            if (rc == 0) {
                return;
            }
            if (errno == EAGAIN || errno == EINTR) {
                return;
            }
        }
    }

    void FutexWake(int count) {
        syscall(
            SYS_futex,
            reinterpret_cast<int*>(&seq_),
            FUTEX_WAKE_PRIVATE,
            count,
            nullptr,
            nullptr,
            0);
    }

    alignas(4) std::atomic<std::uint32_t> seq_{0};
};
