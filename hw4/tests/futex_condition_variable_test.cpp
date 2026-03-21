#include "futex_condition_variable.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

TEST(FutexConditionVariable, NotifyOneWakesSingleWaiter) {
    FutexConditionVariable cv;
    std::mutex mutex;
    bool ready = false;
    std::atomic<int> awakened{0};

    auto waiter = [&]() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.Wait(lock, [&]() { return ready; });
        ++awakened;
    };

    std::thread t1(waiter);
    std::thread t2(waiter);

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {
        std::lock_guard<std::mutex> lock(mutex);
        ready = true;
    }

    cv.NotifyOne();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const int after_one = awakened.load();
    EXPECT_GE(after_one, 1);
    EXPECT_LE(after_one, 2);

    cv.NotifyAll();

    t1.join();
    t2.join();

    EXPECT_EQ(awakened.load(), 2);
}

TEST(FutexConditionVariable, NotifyAllWakesAllWaiters) {
    FutexConditionVariable cv;
    std::mutex mutex;
    bool start = false;
    constexpr int kThreads = 8;
    std::atomic<int> awakened{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&]() {
            std::unique_lock<std::mutex> lock(mutex);
            cv.Wait(lock, [&]() { return start; });
            ++awakened;
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    {
        std::lock_guard<std::mutex> lock(mutex);
        start = true;
    }

    cv.NotifyAll();

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(awakened.load(), kThreads);
}

TEST(FutexConditionVariable, WorksWithProducerConsumerPattern) {
    FutexConditionVariable cv;
    std::mutex mutex;
    bool has_value = false;
    int value = 0;

    std::thread producer([&]() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            value = 42;
            has_value = true;
        }
        cv.NotifyOne();
    });

    int result = 0;
    std::thread consumer([&]() {
        std::unique_lock<std::mutex> lock(mutex);
        cv.Wait(lock, [&]() { return has_value; });
        result = value;
    });

    producer.join();
    consumer.join();

    EXPECT_EQ(result, 42);
}
