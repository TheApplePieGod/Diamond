cmake_minimum_required(VERSION 3.19)

project(
    Diamond
    VERSION 0.1
    DESCRIPTION "A vulkan based 2D rendering engine"
)

include_directories("src/lib/glfw-3.3.2.bin.WIN64/include")
include_directories("src/lib/glm")
include_directories("src/lib/stb")
include_directories("include")
include_directories("C:/VulkanSDK/1.2.162.1/Include")

#add_compile_definitions(DIAMOND_DEBUG)

# build shaders as well
add_custom_command(
    OUTPUT CompileShaders
    COMMAND "CompileShaders.bat"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

file(GLOB SOURCES "src/*.cpp")
# add_executable(Diamond src/testing/main.cpp)
#add_executable(Diamond "${SOURCES}" "src/testing/main.cpp" CompileShaders)
add_library(Diamond ${SOURCES})

find_library(glfw3 glfw3 "src/lib/glfw-3.3.2.bin.WIN64/lib-vc2019")
find_library(vulkan-1 vulkan-1 "C:/VulkanSDK/1.2.162.1/Lib")
target_link_libraries(Diamond "${glfw3}")
target_link_libraries(Diamond "${vulkan-1}")

# Tell C++ compiler to use C++20 features.
target_compile_features(Diamond PUBLIC cxx_std_20)