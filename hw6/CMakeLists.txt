cmake_minimum_required(VERSION 3.16)
project(hw6_threadpool_future LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

add_library(hw6_threadpool INTERFACE)
target_include_directories(hw6_threadpool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hw6_threadpool INTERFACE Threads::Threads)

add_executable(hw6_threadpool_tests tests/thread_pool_test.cpp)
target_link_libraries(hw6_threadpool_tests PRIVATE hw6_threadpool GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(hw6_threadpool_tests)
