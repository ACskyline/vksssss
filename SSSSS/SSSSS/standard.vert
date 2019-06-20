#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_KHR_vulkan_glsl : enable

#include "GlobalInclude.glsl"

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

//vec2 positions[4] = vec2[](
//    vec2(-0.5, -0.5),
//    vec2(-0.5, 0.5),
//    vec2(0.5, 0.5),
//	vec2(0.5, -0.5)
//);
//
//vec3 colors[4] = vec3[](
//    vec3(1.0, 0.0, 0.0),
//    vec3(0.0, 1.0, 0.0),
//    vec3(0.0, 0.0, 1.0),
//	vec3(1.0, 1.0, 0.0)
//);
//
//vec2 uvs[4] = vec2[](
//	vec2(0.0, 0.0),
//	vec2(1.0, 0.0),
//	vec2(1.0, 1.0),
//	vec2(0.0, 1.0)
//);

void main() {
    //gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    //fragColor = colors[gl_VertexIndex];
    gl_Position = passUBO.proj * passUBO.view * objectUBO.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}
