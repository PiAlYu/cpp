cmake_minimum_required(VERSION 3.20)

project(pirogov_sort LANGUAGES CXX)

add_executable(sort_array main.cpp)
target_compile_features(sort_array PRIVATE cxx_std_17)

