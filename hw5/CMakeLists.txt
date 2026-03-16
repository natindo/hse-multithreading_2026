cmake_minimum_required(VERSION 3.16)
project(hw5_ipc_mpsc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Threads REQUIRED)

add_library(hw5_ipc INTERFACE)
target_include_directories(hw5_ipc INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(hw5_ipc INTERFACE Threads::Threads)

add_executable(hw5_producer_demo src/producer_demo.cpp)
target_link_libraries(hw5_producer_demo PRIVATE hw5_ipc)

add_executable(hw5_consumer_demo src/consumer_demo.cpp)
target_link_libraries(hw5_consumer_demo PRIVATE hw5_ipc)
