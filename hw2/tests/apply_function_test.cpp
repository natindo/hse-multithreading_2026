#include "../apply_function.h"

#include <atomic>
#include <vector>

#include <gtest/gtest.h>

TEST(ApplyFunctionTest, AppliesTransformSingleThread) {
  std::vector<int> data{1, 2, 3, 4};

  ApplyFunction<int>(data, [](int& value) { value *= 2; }, 1);

  EXPECT_EQ(data, (std::vector<int>{2, 4, 6, 8}));
}

TEST(ApplyFunctionTest, AppliesTransformMultiThread) {
  std::vector<int> data{1, 2, 3, 4, 5, 6, 7, 8};

  ApplyFunction<int>(data, [](int& value) { value += 3; }, 4);

  EXPECT_EQ(data, (std::vector<int>{4, 5, 6, 7, 8, 9, 10, 11}));
}

TEST(ApplyFunctionTest, UsesElementCountWhenThreadCountTooLarge) {
  std::vector<int> data{10, 20, 30};
  std::atomic<int> calls{0};

  ApplyFunction<int>(
      data,
      [&calls](int& value) {
        ++calls;
        value -= 5;
      },
      100);

  EXPECT_EQ(data, (std::vector<int>{5, 15, 25}));
  EXPECT_EQ(calls.load(), static_cast<int>(data.size()));
}

TEST(ApplyFunctionTest, WorksForEmptyVector) {
  std::vector<int> data;

  ApplyFunction<int>(data, [](int& value) { value = 42; }, 8);

  EXPECT_TRUE(data.empty());
}

TEST(ApplyFunctionTest, NonPositiveThreadCountFallsBackToSingleThread) {
  std::vector<int> data{1, 2, 3};

  ApplyFunction<int>(data, [](int& value) { value += 1; }, 0);

  EXPECT_EQ(data, (std::vector<int>{2, 3, 4}));
}
