#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"
#include "GlobalIncludeVert.glsl"

void main() 
{
    gl_Position = passUBO.proj * passUBO.view * objectUBO.model * vec4(inPosition, 1.0);
}
