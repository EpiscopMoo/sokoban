cmake_minimum_required(VERSION 3.17)

add_executable(Bench bench.cpp)
target_include_directories(Bench PRIVATE ../src)