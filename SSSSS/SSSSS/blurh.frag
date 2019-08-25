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
	vec2 deltaTexCoordU = vec2(1.0 / float(passUBO.widthTex), 0);
	float deltaDepthU = dFdx(depth);
	float stretchU = sceneUBO.stretchAlpha / (depth + sceneUBO.stretchBeta * abs(deltaDepthU));
	vec2 stretchedTexCoordU = stretchU * deltaTexCoordU;
	vec3 sum = vec3(0,0,0);
	for( int i = 0; i < 7; i++ )
	{
		vec3 tap = texture(texSamplerBlurSrc, fragTexCoord + (i-3.0) * stretchedTexCoordU).rgb;
		sum += curve[i] * tap;
	}
	outBlurDst = vec4(sum, 1.0);
}
