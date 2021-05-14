![Logo](logo.png)

# Diamond

A Vulkan based cross platform 2D rendering engine. While Diamond is an 'engine', it is not a standalone application and must be rooted in another application.

## Why Diamond?

Diamond handles Vulkan behind the scenes and exposes an api for a basic 2d application. Ranging from a 2d game to a particle simulation using compute shaders, Diamond can powerfully handle many use cases. Diamond also has optional easy-to-use integration with [ImGui](https://github.com/ocornut/imgui).

## Features
- Highly customizable Vulkan compute shader integration
- Integrated 2D graphics functions designed to ease the process of developing a simple game or application
- Highly customizable graphics pipelines for the more complex applications of the engine
- Compile-time optional and simple integration with ImGui 
- Exposes handles into internal Vulkan and GLFW components for more advanced usage
- Some basic game engine tools such as delta time, fps, and screen sizing 

## Caveats?

- Diamond is currently work in progress and there is a lot which is subject to change or not implemented yet
- Diamond is not completely library independent
- Asynchronous rendering has not been tested but is likely incompatible 

## License
Copyright (C) 2021 [Evan Thompson](https://evanthompson.site/)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

# Getting Started

## Requirements
- C++17 compiler
- CMake 3.14+

## Dependencies
For convenience and best stability, Diamond includes its dependencies with the repo.

- [glfw 3.3.2](https://www.glfw.org/)
- [glm](https://github.com/g-truc/glm) (headers only)
- [imgui](https://github.com/ocornut/imgui) (optional, compiled with engine)
- [stb_image](https://github.com/nothings/stb) (headers only)

All of these libraries in this repo include their respective licenses

## Building
1. Clone the repo
```
$ git clone https://github.com/TheApplePieGod/Diamond
```

2. Open the root directory CMakeLists.txt and follow the instructions provided in the file

3. Open an elevated command prompt (if using the install command) and run the following commands in the root directory of the repo

```
$ mkdir build
$ cd build
```

4. Now run these commands in either 'Debug' or 'Release' depending on your use case

```
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config Release
$ cmake --install . --config Release
```

5. This will install Diamond to your PC and allows easy integration with another CMake project. Alternatively, the built library/exe files are also stored in the build directory under your specified configuration.

To integrate with another CMake project, include the following lines in your project:
```
add_compile_definitions(DIAMOND_IMGUI) # This should be commented out if you built without ImGui integration
set(VulkanBasePath "C:/VulkanSDK/1.2.162.1") # Replace with your vulkan base installation path
find_package(Diamond REQUIRED)
include_directories(${Diamond_DIR}/../../include)
include_directories(${Diamond_DIR}/../../include/lib)
include_directories(${Diamond_DIR}/../../include/lib/glfw/include)
include_directories("${VulkanBasePath}/Include")
target_link_libraries(YourProjectTarget Diamond::Diamond)
```

All of the libraries that are included with Diamond are referenced in the following ways (the last two are already included with diamond.h):
```
#include <Diamond/diamond.h>
#include <imgui/imgui.h>
#include <glm/vec2.hpp>
#include <GLFW/glfw3.h>
```

## Usage

The main.cpp in the Diamond repository contains a few examples which should give some good insight on basic setup and usage of the library. The associated shader files for the examples also have some documentation on how they can be layed out. The two main header files (diamond.h and structures.h) contain extensive documentation on the api and usage of Diamond. Although the documentation goes fairly in depth, it is recommended to have a rudimentary understanding of how rendering APIs like Vulkan or DirectX function, as well as a strong grasp on C++.

---

To use the library, include 'diamond.h' and initialize a new engine instance

```cpp
diamond* Engine = new diamond();
```

Create a basic main function with a simple event loop and cleanup function
```cpp
int main(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "App", "images/default-texture.png");

    while (Engine->IsRunning())
    {
        Engine->BeginFrame(diamond_camera_mode::OrthographicViewportIndependent, glm::vec2(500.f, 500.f), Engine->GenerateViewMatrix(glm::vec2(0.f, 0.f)));

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
    }

    Engine->Cleanup();

    return 0;
}
```

Create a bare bones square drawing on the screen
```cpp
int main(int argc, char** argv)
{
    diamond* Engine = new diamond();
    
    Engine->Initialize(800, 600, "App", "images/default-texture.png");

    diamond_graphics_pipeline_create_info gpCreateInfo = {};
    gpCreateInfo.vertexShaderPath = "shaders/main.vert.spv";
    gpCreateInfo.fragmentShaderPath = "shaders/main.frag.spv";
    gpCreateInfo.maxVertexCount = 100;
    gpCreateInfo.maxIndexCount = 100;
    Engine->CreateGraphicsPipeline(gpCreateInfo);

    while (Engine->IsRunning())
    {
        Engine->BeginFrame(diamond_camera_mode::OrthographicViewportIndependent, glm::vec2(500.f, 500.f), Engine->GenerateViewMatrix(glm::vec2(0.f, 0.f)));

        Engine->SetGraphicsPipeline(0);

        diamond_transform quadTransform;
        quadTransform.location = { 0.f, 0.f };
        quadTransform.rotation = 45.f;
        quadTransform.scale = { 500.f, 500.f };
        Engine->DrawQuad(0, quadTransform);

        Engine->EndFrame({ 0.f, 0.f, 0.f, 1.f });
    }

    Engine->Cleanup();

    return 0;
}
```

## Notes

- Diamond uses the cross platform window manager library [GLFW](https://www.glfw.org/) behind the scenes, so the engine exposes the GLFW window handle in order to be used for input and other window related functionality.
- Because Diamond is built on Vulkan, Vulkan expects .spv compiled shaders. The Diamond repository includes a CompileShaders.bat which uses [glslc](https://github.com/google/shaderc/tree/main/glslc) to compile and is called via CMake at build time, so it is a good script to reuse or use as a basis.

---

### Feel free to submit an issue for any bugs and/or feature requests