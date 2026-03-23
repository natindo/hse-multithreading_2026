cmake_minimum_required(VERSION 3.16)
project(hw2_apply_function LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(apply_function INTERFACE)
target_include_directories(apply_function INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

option(HW2_BUILD_TESTS "Build hw2 tests" ON)
if(HW2_BUILD_TESTS)
  enable_testing()
  find_package(GTest REQUIRED)

  add_executable(apply_function_tests tests/apply_function_test.cpp)
  target_link_libraries(apply_function_tests PRIVATE apply_function GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(apply_function_tests)
endif()

option(HW2_BUILD_BENCHMARKS "Build hw2 benchmarks" ON)
if(HW2_BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)

  add_executable(apply_function_benchmarks benchmarks/apply_function_benchmark.cpp)
  target_link_libraries(apply_function_benchmarks PRIVATE apply_function benchmark::benchmark)
endif()
