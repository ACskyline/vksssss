#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragGeometryNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = passUBO.proj * passUBO.view * objectUBO.model * vec4(inPosition, 1.0);
    fragGeometryNormal = (objectUBO.modelInvTrans * vec4(inNormal, 0.0)).xyz;
    fragTexCoord = inTexCoord;
}
