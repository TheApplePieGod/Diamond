cmake_minimum_required(VERSION 3.14)

project(
    imgui
)

file(GLOB SOURCES "*.cpp")
add_library(imgui ${SOURCES})

# Tell C++ compiler to use C++20 features.
target_compile_features(imgui PUBLIC cxx_std_17)