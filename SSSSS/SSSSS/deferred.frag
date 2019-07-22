#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerDiffuse;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerSpecular;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 2)) uniform sampler2D texSamplerBlurV_0;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 3)) uniform sampler2D texSamplerBlurV_1;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 4)) uniform sampler2D texSamplerBlurV_2;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 5)) uniform sampler2D texSamplerBlurV_3;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 6)) uniform sampler2D texSamplerBlurV_4;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 7)) uniform sampler2D texSamplerBlurV_5;

layout(location = 0) out vec4 outColor;

void main() 
{
	if(sceneUBO.deferredMode == 0)
		outColor = vec4(texture(texSamplerDiffuse, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 1)
		outColor = vec4(texture(texSamplerBlurV_0, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 2)
		outColor = vec4(texture(texSamplerBlurV_1, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 3)
		outColor = vec4(texture(texSamplerBlurV_2, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 4)
		outColor = vec4(texture(texSamplerBlurV_3, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 5)
		outColor = vec4(texture(texSamplerBlurV_4, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 6)
		outColor = vec4(texture(texSamplerBlurV_5, fragTexCoord).rgb, 1.0);
	else if(sceneUBO.deferredMode == 7)
		outColor = vec4(texture(texSamplerSpecular, fragTexCoord).rgb, 1.0);
}
