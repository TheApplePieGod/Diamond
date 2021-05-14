#version 450
#extension GL_ARB_separate_shader_objects : enable

// frame buffer is always layed out this way
layout(binding = 0) uniform frame_buffer_object {
    mat4 viewProj;
} frameBuffer;

// no push constants are used in this example

// this is the custom vertex layout for the particle simulation example
layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = frameBuffer.viewProj * vec4(inPosition, 0.0, 1.0);
    gl_PointSize = 2;
    fragColor = inColor;
}