#include "thread_pool.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

TEST(ThreadPool, ComputesValues) {
    ThreadPool pool(4);

    auto f1 = pool.Submit([](int x, int y) { return x + y; }, 2, 5);
    auto f2 = pool.Submit([]() { return std::string("ok"); });

    EXPECT_EQ(f1.Get(), 7);
    EXPECT_EQ(f2.Get(), "ok");
}

TEST(ThreadPool, SupportsVoidTasks) {
    ThreadPool pool(2);
    std::atomic<int> counter{0};

    auto f = pool.Submit([&counter]() { ++counter; });
    f.Get();

    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPool, PropagatesException) {
    ThreadPool pool(2);

    auto f = pool.Submit([]() -> int { throw std::runtime_error("boom"); });

    EXPECT_THROW({ (void)f.Get(); }, std::runtime_error);
}

TEST(ThreadPool, RunsManyTasks) {
    ThreadPool pool(4);

    constexpr int kTasks = 200;
    std::vector<SimpleFuture<int>> futures;
    futures.reserve(kTasks);

    for (int i = 0; i < kTasks; ++i) {
        futures.push_back(pool.Submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return i * i;
        }));
    }

    long long sum = 0;
    for (int i = 0; i < kTasks; ++i) {
        sum += futures[i].Get();
    }

    long long expected = 0;
    for (int i = 0; i < kTasks; ++i) {
        expected += i * i;
    }

    EXPECT_EQ(sum, expected);
}

TEST(ThreadPool, FutureBecomesInvalidAfterGet) {
    ThreadPool pool(1);

    auto f = pool.Submit([]() { return 10; });
    EXPECT_TRUE(f.Valid());
    EXPECT_EQ(f.Get(), 10);
    EXPECT_FALSE(f.Valid());
}
