#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerColor;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerNormal;

void main() {
	vec3 normalAsColor = fragGeometryNormal * 0.5 + 0.5;
	vec3 tangentAsColor = fragTangent * 0.5 + 0.5;
	vec2 uv = FlipV(fragTexCoord);
	mat3 tangentToWorld = mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragGeometryNormal));
	vec3 normalWorld = normalize(tangentToWorld * normalize((texture(texSamplerNormal, uv).xyz * 2.0 - 1.0)));
	vec3 normalWorldAsColor = normalWorld * 0.5 + 0.5;
	if(sceneUBO.mode == 0)
		outColor = vec4(normalWorldAsColor, 1.0);
	else if(sceneUBO.mode == 1)
		outColor = vec4(normalAsColor, 1.0);
	else if(sceneUBO.mode == 2)
		outColor = vec4(tangentAsColor, 1.0);
	else if(sceneUBO.mode == 3)
		outColor = texture(texSamplerNormal, uv);
	else if(sceneUBO.mode == 4)
		outColor = texture(texSamplerColor, uv);
}
