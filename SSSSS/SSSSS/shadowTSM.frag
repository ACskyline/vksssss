#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 geometryNormalAsColor = normalize(fragGeometryNormal) * 0.5 + 0.5;
	outColor = vec4(gl_FragCoord.z, geometryNormalAsColor);
}
