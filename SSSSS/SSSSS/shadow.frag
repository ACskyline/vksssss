//THIS SHADER IS NOT IN USE ANYMORE BECAUSE WE ARE SKIPPING FRAG STAGE
//BY USING AN EMPTY FRAG SHADER AND USING DEPTH BUFFER AS SHADOW MAP INSTEAD OF COLOR BUFFER

//#version 450
//#extension GL_ARB_separate_shader_objects : enable
//#extension GL_KHR_vulkan_glsl : enable
//
//#include "GlobalInclude.glsl"
//#include "GlobalIncludeFrag.glsl"
//
//layout(location = 0) out vec4 outColor;
//
//void main() 
//{
//	outColor = vec4(gl_FragCoord.z, gl_FragCoord.z, gl_FragCoord.z, 1.0);
//}

//The location of the fragment in window space. The X, Y and Z components are the window-space position of the fragment. 
//The Z value will be written to the depth buffer if gl_FragDepth is not written to by this shader stage. 
//The W component of gl_FragCoord is 1/Wclip, where Wclip is the interpolated W component of the clip-space vertex position output to gl_Position from the last Vertex Processing stage.