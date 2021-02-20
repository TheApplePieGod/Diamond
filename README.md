# Diamond

A Vulkan based cross platform 2D rendering engine. While Diamond is an 'engine', it is not a standalone application and must be rooted in another application.

## Why Diamond?

Diamond handles Vulkan behind the scenes and exposes an api for a basic 2d application. While there is customization, it is not extremely in depth and may not be applicable for all use cases.

## Caveats?

- Diamond is currently work in progress and there is a lot to do in terms of exposing functionality and customization
- Diamond uses the cross platform window manager library [GLFW](https://www.glfw.org/) behind the scenes, so the engine exposes the GLFW data in order to be used for input and other window related functionality
- High level vulkan components are also exposed

### Read through the diamond.h header file for detailed documentation on functionality. Usage/installation instructions coming soon.