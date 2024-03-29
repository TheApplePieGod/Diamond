#version 450
#extension GL_ARB_separate_shader_objects : enable

// frame buffer is always layed out this way
layout(binding = 0) uniform frame_buffer_object {
    mat4 viewProj;
} frameBuffer;

// this is the default push constant struct defined by diamond
layout(push_constant) uniform object_data {
    mat4 model;
    int textureIndex;
} pushConstant;

// this is the default vertex layout defined by diamond
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in int inTextureIndex;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) flat out int fragTextureIndex;

void main() {
    gl_Position = frameBuffer.viewProj * pushConstant.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;

    if (pushConstant.textureIndex == -1)
        fragTextureIndex = inTextureIndex;
    else
        fragTextureIndex = pushConstant.textureIndex;
}