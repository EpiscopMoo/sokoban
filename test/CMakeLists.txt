cmake_minimum_required(VERSION 3.17)

add_executable(Test TestBase.cpp)
target_include_directories(Test PRIVATE ../src)
add_test(NAME TestBase COMMAND Test)