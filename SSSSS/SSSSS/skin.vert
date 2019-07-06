#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeVert.glsl"

void main() 
{
	vec4 positionWorld = objectUBO.model * vec4(inPosition, 1.0);
	fragPosition = positionWorld.xyz;
    gl_Position = passUBO.proj * passUBO.view * positionWorld;
    fragGeometryNormal = (objectUBO.modelInvTrans * vec4(inNormal, 0.0)).xyz;
    fragTexCoord = inTexCoord;
	fragTangent = normalize((objectUBO.model * vec4(inTangent.xyz, 0.0)).xyz);
	fragBitangent = normalize((objectUBO.model * vec4(cross(inNormal, inTangent.xyz) * inTangent.w, 0.0)).xyz);
}
