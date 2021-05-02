cmake_minimum_required(VERSION 3.19)

project(
    imgui
)

include_directories("../glfw-3.3.2.bin.WIN64/include")
include_directories("C:/VulkanSDK/1.2.162.1/Include")

file(GLOB SOURCES "*.cpp")
add_library(imgui ${SOURCES})

# Tell C++ compiler to use C++20 features.
target_compile_features(imgui PUBLIC cxx_std_20)