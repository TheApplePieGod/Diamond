## Read through this file for documented build options and mandatory actions (steps 1-3)

cmake_minimum_required(VERSION 3.14)

project(
    Diamond
    VERSION 1.0
    DESCRIPTION "A vulkan based 2D rendering engine"
)

include_directories("src/lib/glfw/include")
include_directories("src/lib")
include_directories("include")

## 1. Build options (these can be set here but it is recommended to set these from the command line,
## as changing the values here doesn't always force a rebuild)
option(DEBUG_MODE "Build Diamond in debug mode" ON)
option(IMGUI_INTEGRATION "Build diamond with ImGui integration" ON)

## 2. Install the Vulkan SDK (major version 1.2) from https://vulkan.lunarg.com/sdk/home and replace the base path below
set(VulkanBasePath "C:/VulkanSDK/1.2.162.1")
include_directories("${VulkanBasePath}/Include")
find_library(vulkan-1 vulkan-1 "${VulkanBasePath}/Lib")

if (DEBUG_MODE)
    add_compile_definitions(DIAMOND_DEBUG)
endif()

file(GLOB SOURCES "src/*.cpp")

## 3. To compile examples, uncomment add_executable and comment out add_library and rebuild
## otherwise, change nothing
# add_executable(Diamond "${SOURCES}" "src/testing/main.cpp")
add_library(Diamond ${SOURCES})

# build shaders as well
add_custom_command(
    TARGET Diamond POST_BUILD
    COMMAND "CompileShaders.bat"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

if (IMGUI_INTEGRATION)
    add_subdirectory("src/lib/imgui")
    add_compile_definitions(DIAMOND_IMGUI)
    target_link_libraries(Diamond imgui)
    install(
        TARGETS imgui
        EXPORT DiamondTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

find_library(glfw3 glfw3 "src/lib/glfw/lib-vc2019")
target_link_libraries(Diamond "${glfw3}")
target_link_libraries(Diamond "${vulkan-1}")

# Install
include(GNUInstallDirs)
install(
    TARGETS Diamond
    EXPORT DiamondTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    DIRECTORY "include/Diamond"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/include
    FILES_MATCHING
    PATTERN "*.h"
)
install(
    DIRECTORY "src/lib"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/include
    FILES_MATCHING
    PATTERN "*.h"
    PATTERN "*.hpp"
    PATTERN "*.inl"
)
install(
    EXPORT DiamondTargets
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Diamond
    NAMESPACE Diamond::
    FILE DiamondTargets.cmake
)
include(CMakePackageConfigHelpers)
configure_package_config_file( 
    "Config.cmake.in" 
    "DiamondConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Diamond
    PATH_VARS
    CMAKE_INSTALL_LIBDIR
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/DiamondConfigVersion.cmake
    VERSION 1.0.0
    COMPATIBILITY SameMajorVersion
)
install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/DiamondConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/DiamondConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Diamond"
)

target_compile_features(Diamond PUBLIC cxx_std_17)