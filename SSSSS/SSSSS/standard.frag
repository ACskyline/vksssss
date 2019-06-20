#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerAlbedo;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    //outColor = vec4(fragTexCoord, 0.0, 1.0);
    outColor = vec4(fragColor, 1) * texture(texSamplerAlbedo, fragTexCoord);
}
