cmake_minimum_required(VERSION 3.7)
project(ex1)

set(CMAKE_CXX_STANDARD 98)

set(SOURCE_FILES ex12.c)
add_executable(ex1 ${SOURCE_FILES})
add_executable(comp.out ex11.c)