#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeFrag.glsl"

layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 0)) uniform sampler2D texSamplerColor;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 1)) uniform sampler2D texSamplerNormal;
layout(set = PASS_SET, binding = TEXTURE_SLOT(PASS, 2)) uniform sampler2D texSamplerTransmitanceMask;

layout(set = SCENE_SET, binding = TEXTURE_SLOT(SCENE, 0)) uniform sampler2DShadow lightTextureArrayShadow[MAX_LIGHTS_PER_SCENE];
layout(set = SCENE_SET, binding = TEXTURE_SLOT(SCENE, 1)) uniform sampler2D lightTextureArray[MAX_LIGHTS_PER_SCENE];

layout(location = 0) out vec4 outDiffuse;
layout(location = 1) out vec4 outSpecular;

void main()
{
	vec3 geometryNormalAsColor = normalize(fragGeometryNormal) * 0.5 + 0.5;
	vec3 tangentAsColor = normalize(fragTangent) * 0.5 + 0.5;
	vec2 uv = FlipV(fragTexCoord);
	//TBN have to be re-normalized because of the fragment interpolation 
	mat3 tangentToWorld = mat3(normalize(fragTangent), normalize(fragBitangent), normalize(fragGeometryNormal));
	vec3 normal = texture(texSamplerNormal, uv).xyz;
	vec3 normalWorld = normalize(tangentToWorld * normalize((normal * 2.0 - 1.0)));
	vec3 normalWorldAsColor = normalWorld * 0.5 + 0.5;
	vec3 albedo = texture(texSamplerColor, uv).rgb;
	vec3 transmitanceMask = texture(texSamplerTransmitanceMask, uv).rgb;
	vec3 cameraDirWorld = normalize(passUBO.cameraPosition.xyz - fragPosition);
	
	vec3 diffuseTotal = vec3(0, 0, 0);
	vec3 specularTotal = vec3(0, 0, 0);

	for(uint i = 0;i<sceneUBO.lightCount;i++)
	{
		float shadow = 0;
		vec4 TSM = vec4(0,0,0,0);//r = shadow, gba = translucency
		
		if(sceneUBO.shadowMode == 5)
		{
			TSM = ShadowFeelerTSM(
						cameraDirWorld,
						transmitanceMask,
						fragPosition, 
						normalWorld, 
						sceneUBO.lightArr[i].position.xyz,
						sceneUBO.lightArr[i].color.xyz,
						sceneUBO.lightArr[i].near,
						sceneUBO.lightArr[i].far,
						sceneUBO.lightArr[i].view,
						sceneUBO.lightArr[i].proj,
						sceneUBO.lightArr[i].viewInv,
						sceneUBO.lightArr[i].projInv,
						lightTextureArray[sceneUBO.lightArr[i].textureIndex]);
			vec3 lightDir = normalize(sceneUBO.lightArr[i].position.xyz - fragPosition);
			vec3 distortedNormal = - (lightDir + sceneUBO.distortion *
												transmitanceMask *
												normalWorld);
			TSM.gba *= pow(clamp(transmitanceMask.r * dot(cameraDirWorld, distortedNormal), 0, 1), sceneUBO.translucencyPower) * sceneUBO.lightArr[i].color.xyz;
			shadow = TSM.r;
		}
		else if(sceneUBO.shadowMode == 4)
		{
		    vec3 lightDir = normalize(sceneUBO.lightArr[i].position.xyz - fragPosition);
			vec3 distortedNormal = - (lightDir + sceneUBO.distortion *
												transmitanceMask *
												normalWorld);
			TSM.gba = pow(clamp(transmitanceMask.r * dot(cameraDirWorld, distortedNormal), 0, 1), sceneUBO.translucencyPower) * sceneUBO.lightArr[i].color.xyz;
			shadow = ShadowFeelerPCF_7X7(
						sceneUBO.lightArr[i].view,
						sceneUBO.lightArr[i].proj,
						fragPosition,
						lightTextureArrayShadow[sceneUBO.lightArr[i].textureIndex]);
		}
		else if(sceneUBO.shadowMode == 3)
		{
			TSM = ShadowFeelerTSM(
						cameraDirWorld,
						transmitanceMask,
						fragPosition, 
						normalWorld, 
						sceneUBO.lightArr[i].position.xyz,
						sceneUBO.lightArr[i].color.xyz,
						sceneUBO.lightArr[i].near,
						sceneUBO.lightArr[i].far,
						sceneUBO.lightArr[i].view,
						sceneUBO.lightArr[i].proj,
						sceneUBO.lightArr[i].viewInv,
						sceneUBO.lightArr[i].projInv,
						lightTextureArray[sceneUBO.lightArr[i].textureIndex]);
			shadow = TSM.r;
		}
		else if (sceneUBO.shadowMode == 2)
			shadow = ShadowFeelerPCF_7X7(
						sceneUBO.lightArr[i].view,
						sceneUBO.lightArr[i].proj,
						fragPosition,
						lightTextureArrayShadow[sceneUBO.lightArr[i].textureIndex]);
		else if (sceneUBO.shadowMode == 1)
			shadow = ShadowFeelerPCF(
						sceneUBO.lightArr[i].view,
						sceneUBO.lightArr[i].proj,
						fragPosition,
						lightTextureArrayShadow[sceneUBO.lightArr[i].textureIndex]);
		else 
			shadow = ShadowFeeler(
						sceneUBO.lightArr[i].view,
						sceneUBO.lightArr[i].proj,
						fragPosition,
						lightTextureArray[sceneUBO.lightArr[i].textureIndex]);

		vec3 lightDirWorld = normalize(sceneUBO.lightArr[i].position.xyz - fragPosition);

		diffuseTotal +=
			albedo * 
			transmitanceMask *
			TSM.gba *
			sceneUBO.translucencyScale
			+
			albedo *
			sceneUBO.lightArr[i].color.rgb * 
			clamp(dot(lightDirWorld, normalWorld), 0, 1) * 
			shadow *
			sceneUBO.shadowScale;
			
		specularTotal += 
			sceneUBO.lightArr[i].color.rgb *
			KS_Skin_Specular(
				normalWorld, 
				lightDirWorld,
				cameraDirWorld,
				sceneUBO.m,
				sceneUBO.rho_s) * 
			shadow;
	}

	outDiffuse = vec4(diffuseTotal, gl_FragCoord.z);
	outSpecular = vec4(specularTotal, 1.0);
}
