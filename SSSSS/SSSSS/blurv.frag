#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerBlurSrc;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerDiffuse;

layout(location = 0) out vec4 outBlurDst;
 
float curve[7] = {0.006,0.061,0.242,0.383,0.242,0.061,0.006};

void main()
{
	float depth = texture(texSamplerDiffuse, fragTexCoord).w;
	vec2 deltaTexCoordV = vec2(0, 1.0 / passUBO.heightTex);
	float deltaDepthV = dFdy(depth);
	float stretchV = sceneUBO.stretchAlpha / (depth + sceneUBO.stretchBeta * abs(deltaDepthV));
	vec2 stretchedTexCoordV = stretchV * deltaTexCoordV;
	vec3 sum = vec3(0,0,0);
	for( int i = 0; i < 7; i++ )
	{
		vec3 tap = texture(texSamplerBlurSrc, fragTexCoord + (i-3.0) * stretchedTexCoordV).rgb;
		sum += curve[i] * tap;
	}
	outBlurDst = vec4(sum, 1.0);
}
