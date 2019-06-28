#define SCENE_SET 0//per scene
#define FRAME_SET 1//per frame
#define PASS_SET 2//per pass
#define OBJECT_SET 3//per draw call

#define SCENE_UBO_BINDING_COUNT 1

#define FRAME_UBO_BINDING_COUNT 1

#define PASS_UBO_BINDING_COUNT 1

#define OBJECT_UBO_BINDING_COUNT 1

#define UBO_SLOT(NAME, NUMBER) (NUMBER)
#define TEXTURE_SLOT(NAME, NUMBER) NAME ## _UBO_BINDING_COUNT + NUMBER

layout(set = SCENE_SET, binding = UBO_SLOT(SCENE, 0)) uniform SceneUniformBufferObject {
    uint time;
	uint mode;
} sceneUBO;

layout(set = FRAME_SET, binding = UBO_SLOT(FRAME, 0)) uniform FrameUniformBufferObject {
    uint frameNum;
} frameUBO;

layout(set = PASS_SET, binding = UBO_SLOT(PASS, 0)) uniform PassUniformBufferObject {
	mat4 view;
	mat4 proj;
	uint passNum;
} passUBO;

layout(set = OBJECT_SET, binding = UBO_SLOT(OBJECT, 0)) uniform ObjectUniformBufferObject {
    mat4 model;
	mat4 modelInvTrans;
} objectUBO;

vec2 FlipV(vec2 uv)
{
	return vec2(uv.x, 1.0 - uv.y);
}