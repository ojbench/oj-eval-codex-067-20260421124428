cmake_minimum_required(VERSION 3.12)
project(problem_067_refcell CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(code main.cpp)
target_compile_options(code PRIVATE -O2 -pipe -static -s)
