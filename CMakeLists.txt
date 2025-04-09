cmake_minimum_required(VERSION 3.10.0)
project(task_30.7.1 VERSION 0.1.0 LANGUAGES C CXX)

add_executable(task_30.7.1 main.cpp quicksort.cpp task_context.cpp thread_pool.cpp)

include(CTest)
enable_testing()

