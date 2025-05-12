cmake_minimum_required(VERSION 3.28)
project(ThreadPool)

set(CMAKE_CXX_STANDARD 17)

add_library(ThreadPool SHARED thread_pool.cpp)
