#pragma once

#include "GlobalInclude.h"

class Renderer;
class Texture;

class Frame
{
public:
	Frame();
	Frame(const std::string& _name);
	~Frame();

	uint32_t GetTextureCount() const;
	uint32_t GetUboCount() const;
	const std::vector<Texture*>& GetTextureVec() const;
	VkBuffer GetFrameUniformBuffer() const;
	const FrameUniformBufferObject& GetFrameUniformBufferObject() const;
	VkDescriptorSet* GetFrameDescriptorSetPtr();
	VkDescriptorSetLayout GetFrameDescriptorSetLayout() const;

	void AddTexture(Texture* pTexture);

	void InitFrame(Renderer* _pRenderer, VkDescriptorPool descriptorPool);
	void UpdateFrameUniformBuffer();

	void CleanUp();

private:
	Renderer* pRenderer;
	std::string name;
	const uint32_t fUboCount = 1;

	//asset containers
	std::vector<Texture*> pTextureVec;

	//frame uniform
	FrameUniformBufferObject fUBO;
	VkBuffer frameUniformBuffer;
	VkDeviceMemory frameUniformBufferMemory;
	VkDescriptorSet frameDescriptorSet;
	VkDescriptorSetLayout frameDescriptorSetLayout;

	void CreateFrameUniformBuffer();
};
