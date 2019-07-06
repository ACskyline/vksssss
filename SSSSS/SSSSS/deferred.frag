#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerDiffuse;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerSpecular;

layout(location = 0) out vec4 outColor;

void main() 
{
	if(sceneUBO.deferredMode == 0)
		outColor = vec4(texture(texSamplerDiffuse, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 1)
		outColor = vec4(texture(texSamplerSpecular, fragTexCoord).rgb, 1.0);
}
