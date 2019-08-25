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
	vec3 diffuseCol = texture(texSamplerDiffuse, fragTexCoord).rgb;
	vec3 specularCol = texture(texSamplerSpecular, fragTexCoord).rgb;
	vec3 blurV_0 = texture(texSamplerBlurV_0, fragTexCoord).rgb;
	vec3 blurV_1 = texture(texSamplerBlurV_1, fragTexCoord).rgb;
	vec3 blurV_2 = texture(texSamplerBlurV_2, fragTexCoord).rgb;
	vec3 blurV_3 = texture(texSamplerBlurV_3, fragTexCoord).rgb;
	vec3 blurV_4 = texture(texSamplerBlurV_4, fragTexCoord).rgb;
	vec3 blurV_5 = texture(texSamplerBlurV_5, fragTexCoord).rgb;

	if(sceneUBO.deferredMode == 0)
		outColor = vec4(diffuseCol, 1.0);
	else if(sceneUBO.deferredMode == 1)
		outColor = vec4(blurV_0, 1.0);
	else if(sceneUBO.deferredMode == 2)
		outColor = vec4(blurV_1, 1.0);
	else if(sceneUBO.deferredMode == 3)
		outColor = vec4(blurV_2, 1.0);
	else if(sceneUBO.deferredMode == 4)
		outColor = vec4(blurV_3, 1.0);
	else if(sceneUBO.deferredMode == 5)
		outColor = vec4(blurV_4, 1.0);
	else if(sceneUBO.deferredMode == 6)
		outColor = vec4(blurV_5, 1.0);
	else if(sceneUBO.deferredMode == 7)
		outColor = vec4(specularCol, 1.0);
	else if(sceneUBO.deferredMode >= 8)
	{
		vec3 weight_0 = vec3(0.233, 0.455, 0.649);
		vec3 weight_1 = vec3(0.1, 0.336, 0.344);
		vec3 weight_2 = vec3(0.118, 0.198, 0);
		vec3 weight_3 = vec3(0.113, 0.007, 0.007);
		vec3 weight_4 = vec3(0.358, 0.004, 0);
		vec3 weight_5 = vec3(0.078, 0, 0);

		outColor = vec4(
			weight_0 * blurV_0 +
			weight_1 * blurV_1 +
			weight_2 * blurV_2 + 
			weight_3 * blurV_3 +
			weight_4 * blurV_4 +
			weight_5 * blurV_5 + 
			specularCol,
			1.0);
	}
}
