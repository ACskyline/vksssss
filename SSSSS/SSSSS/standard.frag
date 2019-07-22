#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerColor;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerNormal;

layout(set = SCENE_SET, binding = TEXTURE_SLOT(SCENE, 0)) uniform sampler2D lightTextureArray[MAX_LIGHTS_PER_SCENE];

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;

void main()
{
	vec3 normalAsColor = normalize(fragGeometryNormal) * 0.5 + 0.5;
	vec3 tangentAsColor = normalize(fragTangent) * 0.5 + 0.5;
	vec2 uv = FlipV(fragTexCoord);
	//TBN have to be re-normalized because of the fragment interpolation 
	mat3 tangentToWorld = mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragGeometryNormal));
	vec3 normal = texture(texSamplerNormal, uv).xyz;
	vec3 normalWorld = normalize(tangentToWorld * normalize((normal * 2.0 - 1.0)));
	vec3 normalWorldAsColor = normalWorld * 0.5 + 0.5;
	vec3 albedo = texture(texSamplerColor, uv).rgb;
	
	outDiffuse = vec4(albedo, gl_FragCoord.z);

	vec3 specularTotal = vec3(0, 0, 0);
	for(uint i = 0;i<sceneUBO.lightCount;i++)
	{
		specularTotal += 
			sceneUBO.lightArr[i].color.rgb *
//			KS_Skin_Specular(
//				normalWorld, 
//				normalize(sceneUBO.lightArr[i].position.xyz - fragPosition),
//				normalize(passUBO.cameraPosition.xyz - fragPosition),
//				sceneUBO.m,
//				sceneUBO.rho_s) * 
			ShadowFeeler(
				sceneUBO.lightArr[i].view,
				sceneUBO.lightArr[i].proj,
				fragPosition,
				lightTextureArray[sceneUBO.lightArr[i].textureIndex]);
	}

	outSpecular = vec4(specularTotal, 1.0);
}
