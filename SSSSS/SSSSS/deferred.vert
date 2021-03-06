#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeVert.glsl"

void main() 
{
    gl_Position = vec4(inPosition, 1.0);
	fragPosition = inPosition;
    fragGeometryNormal = inNormal;
	fragTangent = inTangent.xyz;
	fragBitangent = cross(inNormal, inTangent.xyz) * inTangent.w;
    fragTexCoord = inTexCoord;
}
