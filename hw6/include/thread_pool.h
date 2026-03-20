#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

template <class T>
class SimpleFuture {
public:
    SimpleFuture() = default;

    T Get() {
        auto state = state_;
        if (!state) {
            throw std::runtime_error("future has no state");
        }

        std::unique_lock<std::mutex> lock(state->mutex);
        state->cv.wait(lock, [&]() { return state->ready; });

        if (state->exception) {
            std::rethrow_exception(state->exception);
        }

        T value = std::move(*state->value);
        state_.reset();
        return value;
    }

    bool Valid() const {
        return static_cast<bool>(state_);
    }

private:
    struct State {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        std::optional<T> value;
        std::exception_ptr exception;
    };

    explicit SimpleFuture(std::shared_ptr<State> state)
        : state_(std::move(state)) {
    }

    std::shared_ptr<State> state_;

    template <class U>
    friend class Promise;
};

template <>
class SimpleFuture<void> {
public:
    SimpleFuture() = default;

    void Get() {
        auto state = state_;
        if (!state) {
            throw std::runtime_error("future has no state");
        }

        std::unique_lock<std::mutex> lock(state->mutex);
        state->cv.wait(lock, [&]() { return state->ready; });

        if (state->exception) {
            std::rethrow_exception(state->exception);
        }

        state_.reset();
    }

    bool Valid() const {
        return static_cast<bool>(state_);
    }

private:
    struct State {
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;
        std::exception_ptr exception;
    };

    explicit SimpleFuture(std::shared_ptr<State> state)
        : state_(std::move(state)) {
    }

    std::shared_ptr<State> state_;

    template <class U>
    friend class Promise;
};

template <class T>
class Promise {
public:
    Promise()
        : state_(std::make_shared<typename SimpleFuture<T>::State>()) {
    }

    SimpleFuture<T> GetFuture() {
        return SimpleFuture<T>(state_);
    }

    void SetValue(T value) {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->ready) {
            throw std::runtime_error("promise already satisfied");
        }
        state_->value = std::move(value);
        state_->ready = true;
        state_->cv.notify_all();
    }

    void SetException(std::exception_ptr exception) {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->ready) {
            throw std::runtime_error("promise already satisfied");
        }
        state_->exception = std::move(exception);
        state_->ready = true;
        state_->cv.notify_all();
    }

private:
    std::shared_ptr<typename SimpleFuture<T>::State> state_;
};

template <>
class Promise<void> {
public:
    Promise()
        : state_(std::make_shared<typename SimpleFuture<void>::State>()) {
    }

    SimpleFuture<void> GetFuture() {
        return SimpleFuture<void>(state_);
    }

    void SetValue() {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->ready) {
            throw std::runtime_error("promise already satisfied");
        }
        state_->ready = true;
        state_->cv.notify_all();
    }

    void SetException(std::exception_ptr exception) {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->ready) {
            throw std::runtime_error("promise already satisfied");
        }
        state_->exception = std::move(exception);
        state_->ready = true;
        state_->cv.notify_all();
    }

private:
    std::shared_ptr<typename SimpleFuture<void>::State> state_;
};

class ThreadPool {
public:
    explicit ThreadPool(std::size_t thread_count)
        : stop_(false) {
        if (thread_count == 0) {
            throw std::invalid_argument("thread_count must be > 0");
        }
        workers_.reserve(thread_count);
        for (std::size_t i = 0; i < thread_count; ++i) {
            workers_.emplace_back([this]() { WorkerLoop(); });
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            worker.join();
        }
    }

    template <class F, class... Args>
    auto Submit(F&& f, Args&&... args)
        -> SimpleFuture<std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>> {
        using Result = std::invoke_result_t<std::decay_t<F>, std::decay_t<Args>...>;

        auto task = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        Promise<Result> promise;
        auto future = promise.GetFuture();

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (stop_) {
                throw std::runtime_error("submit on stopped thread pool");
            }

            tasks_.emplace_back([task = std::move(task), promise = std::move(promise)]() mutable {
                try {
                    if constexpr (std::is_void_v<Result>) {
                        task();
                        promise.SetValue();
                    } else {
                        promise.SetValue(task());
                    }
                } catch (...) {
                    promise.SetException(std::current_exception());
                }
            });
        }

        cv_.notify_one();
        return future;
    }

private:
    void WorkerLoop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                cv_.wait(lock, [this]() { return stop_ || !tasks_.empty(); });

                if (stop_ && tasks_.empty()) {
                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop_front();
            }

            task();
        }
    }

    std::mutex mutex_;
    std::condition_variable cv_;
    std::deque<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    bool stop_;
};
