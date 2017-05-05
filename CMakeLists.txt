cmake_minimum_required(VERSION 3.7)
project(ex1)

set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES ex11.c ex12.c)
add_executable(ex1 ${SOURCE_FILES})