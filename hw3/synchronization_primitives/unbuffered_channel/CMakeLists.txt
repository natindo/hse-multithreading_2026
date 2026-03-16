cmake_minimum_required(VERSION 3.25)
project(unbuffered_channel LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(unbuffered_channel INTERFACE)
target_include_directories(unbuffered_channel INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(unbuffered_channel INTERFACE Threads::Threads)

option(UNBUFFERED_CHANNEL_BUILD_TESTS "Build unbuffered_channel tests" ON)
if(UNBUFFERED_CHANNEL_BUILD_TESTS)
  enable_testing()
  find_package(GTest REQUIRED)

  add_executable(unbuffered_channel_tests test.cpp)
  target_link_libraries(unbuffered_channel_tests
    PRIVATE
      unbuffered_channel
      GTest::gtest_main
  )

  include(GoogleTest)
  gtest_discover_tests(unbuffered_channel_tests)
endif()

option(UNBUFFERED_CHANNEL_BUILD_BENCHMARKS "Build unbuffered_channel benchmarks" ON)
if(UNBUFFERED_CHANNEL_BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)

  add_executable(unbuffered_channel_benchmarks benchmark.cpp)
  target_link_libraries(unbuffered_channel_benchmarks
    PRIVATE
      unbuffered_channel
      benchmark::benchmark
  )
endif()