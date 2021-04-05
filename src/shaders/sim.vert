#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform frame_buffer_object {
    mat4 viewProj;
} frameBuffer;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = frameBuffer.viewProj * vec4(inPosition, 0.0, 1.0);
    gl_PointSize = 10;
    fragColor = inColor;
}