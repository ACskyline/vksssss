#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerColor;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerNormal;

layout(location = 0) in vec3 fragGeometryNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
	vec3 normalAsColor = fragGeometryNormal * 0.5 + 0.5;
	vec2 uv = FlipV(fragTexCoord);
    outColor = vec4(normalAsColor, 1.0);//texture(texSamplerColor, uv);
}
