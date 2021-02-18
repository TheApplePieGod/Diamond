#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform frame_buffer_object {
    mat4 viewProj;
} frameBuffer;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int textureIndex;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out int fragTextureIndex;

void main() {
    gl_Position = frameBuffer.viewProj * vec4(inPosition, 0.0, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
    fragTextureIndex = textureIndex;
}