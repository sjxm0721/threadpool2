cmake_minimum_required(VERSION 3.23)
project(threadpool2)

set(CMAKE_CXX_STANDARD 11)

link_libraries(pthread)

add_executable(threadpool2 main.cpp threadpool.cpp threadpool.h)
