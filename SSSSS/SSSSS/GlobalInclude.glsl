#define SCENE_SET 0//per scene
#define FRAME_SET 1//per frame
#define PASS_SET 2//per pass
#define OBJECT_SET 3//per draw call

#define SCENE_UBO_BINDING_OFFSET 0
#define SCENE_TEXTURE_BINDING_OFFSET 1

#define FRAME_UBO_BINDING_OFFSET 0
#define FRAME_TEXTURE_BINDING_OFFSET 1

#define PASS_UBO_BINDING_OFFSET 0
#define PASS_TEXTURE_BINDING_OFFSET 1

#define OBJECT_UBO_BINDING_OFFSET 0
#define OBJECT_TEXTURE_BINDING_OFFSET 1

#define UBO_SLOT(NAME, NUMBER) (NAME ## _UBO_BINDING_OFFSET + NUMBER)
#define TEXTURE_SLOT(NAME, NUMBER) NAME ## _TEXTURE_BINDING_OFFSET + NUMBER

layout(set = SCENE_SET, binding = UBO_SLOT(SCENE, 0)) uniform SceneUniformBufferObject {
    uint time;
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
} objectUBO;