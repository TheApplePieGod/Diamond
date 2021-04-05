#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(binding = 1) uniform sampler2D texSampler[];

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) flat in int fragTextureIndex;

layout(location = 0) out vec4 outColor;

void main() {
    if (fragTextureIndex < 0)
        outColor = fragColor;
    else
        outColor = fragColor * texture(texSampler[fragTextureIndex], fragTexCoord).rgba;
    //outColor = vec4(1.f,1.f,1.f,1.f);
}