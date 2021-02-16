## Diamond

A Vulkan based cross platform 2D rendering engine. While Diamond is an 'engine', it is not a standalone application and must be rooted in another application.

# Why Diamond?

Diamond handles Vulkan behind the scenes and exposes enough funtionality and customization to support a 2D game engine.

# Caveats?

- Diamond is currently not at all finished and has zero functionality (yet)
- Diamond uses the cross platform window manager library [GLFW](https://www.glfw.org/) behind the scenes, so the engine exposes the GLFW data in order to be used for input and other window related functionality 