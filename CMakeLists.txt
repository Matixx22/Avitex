cmake_minimum_required(VERSION 3.17)
project(Avitex)

set(CMAKE_CXX_STANDARD 20)

add_executable(Avitex main.cpp)
add_library(Avitex_lib STATIC lib/Utils.cpp)