cmake_minimum_required(VERSION 3.20)
project(agent_demo LANGUAGES CXX)

add_executable(demo_app main.cpp)
target_compile_features(demo_app PRIVATE cxx_std_17)
