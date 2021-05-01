# Diamond

A Vulkan based cross platform 2D rendering engine. While Diamond is an 'engine', it is not a standalone application and must be rooted in another application.

## Why Diamond?

Diamond handles Vulkan behind the scenes and exposes an api for a basic 2d application. Ranging from a 2d game to a particle simulation using compute shaders, Diamond can powerfully handle many use cases. Diamond also has optional easy-to-use integration with [ImGui](https://github.com/ocornut/imgui).

## Caveats?

- Diamond is currently work in progress and there is a lot which is subject to change or not implemented yet
- Diamond is not completely library dependent

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

## Building
Install CMake

Clone the repo
```
$ git clone https://github.com/TheApplePieGod/Diamond
```

Open the root directory CMakeLists.txt and follow the instructions provided

Run the following commands in the root directory of the repo

```
$ mkdir build
$ cd build
```

Now run these two commands in either 'Debug' or 'Release' depending on your use case

```
$ cmake .. -DCMAKE_BUILD_TYPE=Release
$ cmake --build . --config Release
```

This build directory will now contain your built files in either the 'Debug' or 'Release' subdirectories

## Usage

The main.cpp in the Diamond repository should give some good insight on basic setup and usage of the library. The two main header files (diamond.h and structures.h) contain extensive documentation on the api and usage of the library. Although the documentation goes fairly in depth, it is recommended to have a rudimentary understanding of CMake and how rendering APIs like Vulkan or DirectX function, as well as a strong grasp on C++.

---

To use the library, include 'Diamond.h' located in the include directory and initialize a new engine instance

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

- Diamond uses the cross platform window manager library [GLFW](https://www.glfw.org/) behind the scenes, so the engine exposes the GLFW data in order to be used for input and other window related functionality.
- Because Diamond is built on Vulkan, Vulkan expects .spv compiled shaders. The Diamond repository includes a CompileShaders.bat which uses [glslc](https://github.com/google/shaderc/tree/main/glslc) to compile and is called via CMake at build time, so it is a good script to reuse or use as a basis.