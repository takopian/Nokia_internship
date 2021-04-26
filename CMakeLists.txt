cmake_minimum_required(VERSION 3.10)

project(CSV)
set(CMAKE_CXX_STANDARD 17)

add_executable(csv-reader src/solution.cpp src/table.hpp)