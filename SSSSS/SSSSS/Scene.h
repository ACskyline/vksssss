#pragma once

#include "GlobalInclude.h"

class Renderer;
class Pass;
class Texture;
class Light;

class Scene
{
public:
	Scene(const std::string& _name);
	~Scene();

	uint32_t GetUboCount() const;
	const std::vector<Pass*>& GetPassVec() const;
	uint32_t GetTextureCount() const;
	VkBuffer GetSceneUniformBuffer() const;
	const SceneUniformBufferObject& GetSceneUniformBufferObject() const;
	VkDescriptorSet* GetSceneDescriptorSetPtr(int frame);
	int GetSceneDescriptorSetCount() const;
	VkDescriptorSetLayout GetSceneDescriptorSetLayout() const;

	void AddPass(Pass* pPass);
	void AddLight(Light* pLight);

	void InitScene(Renderer* _pRenderer, VkDescriptorPool descriptorPool);

	void CleanUp();

	//vulkan functions
	void UpdateSceneUniformBuffer(int frame);

	//scene uniform
	SceneUniformBufferObject sUBO;

private:
	Renderer* pRenderer;
	std::string name;
	const uint32_t sUboCount = 1;

	//asset containers
	std::vector<Pass*> pPassVec;
	std::vector<Texture*> pTextureVec;//this is a texture array (currently for lights only) instead of multiple textures in the shader
	std::vector<Texture*> pTextureVec2;//this is a 2nd texture array (currently for lights only) instead of multiple textures in the shader
	std::vector<Light*> pLightVec;

	//scene uniform
	VkBuffer sceneUniformBuffer;
	VkDeviceMemory sceneUniformBufferMemory;
	std::vector<VkDescriptorSet> sceneDescriptorSetVec;
	VkDescriptorSetLayout sceneDescriptorSetLayout;

	//vulkan functions
	void CreateSceneUniformBuffer(int frameCount);
};
