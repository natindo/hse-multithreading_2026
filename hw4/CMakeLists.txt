cmake_minimum_required(VERSION 3.16)
project(hw4_futex_condition_variable LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)
find_package(GTest REQUIRED)

enable_testing()

add_library(hw4_futex_cv INTERFACE)
target_include_directories(hw4_futex_cv INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(hw4_futex_cv INTERFACE Threads::Threads)

add_executable(hw4_futex_cv_tests tests/futex_condition_variable_test.cpp)
target_link_libraries(hw4_futex_cv_tests PRIVATE hw4_futex_cv GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(hw4_futex_cv_tests)
