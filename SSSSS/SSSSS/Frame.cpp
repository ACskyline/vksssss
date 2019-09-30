#include "Frame.h"

#include "Renderer.h"
#include "Texture.h"

Frame::Frame() : Frame("unnamed frame")
{
}

Frame::Frame(const std::string& _name) :
	pRenderer(nullptr), name(_name)
{
	fUBO.frameNum = 0;
}

Frame::~Frame()
{
	CleanUp();
}

void Frame::InitFrame(Renderer* _pRenderer, VkDescriptorPool descriptorPool)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("frame " + name + " : pRenderer is null!");
	}
	pRenderer = _pRenderer;

	//create uniform resources
	CreateFrameUniformBuffer();
	pRenderer->CreateDescriptorSetLayout(
		frameDescriptorSetLayout,
		0,
		fUboCount,//only 1 fUBO
		fUboCount,//only 1 fUBO, so offset is 1
		static_cast<uint32_t>(pTextureVec.size()));
	pRenderer->CreateDescriptorSet(
		frameDescriptorSet,
		descriptorPool,
		frameDescriptorSetLayout);

	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(frameUniformBuffer, 0, sizeof(fUBO), { frameDescriptorSet }, 0);//only 1 fUBO

	//bind texture
	for (int i = 0; i < pTextureVec.size(); i++)
	{
		pRenderer->BindTextureToDescriptorSets(pTextureVec[i]->GetTextureImageView(), pTextureVec[i]->GetSampler(), { frameDescriptorSet }, fUboCount + i);//only 1 fUBO, so offset is 1
	}
}

void Frame::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyDescriptorSetLayout(pRenderer->GetDevice(), frameDescriptorSetLayout, nullptr);
		vkDestroyBuffer(pRenderer->GetDevice(), frameUniformBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), frameUniformBufferMemory, nullptr);
		pRenderer = nullptr;
	}
}

uint32_t Frame::GetTextureCount() const
{
	return static_cast<uint32_t>(pTextureVec.size());
}

uint32_t Frame::GetUboCount() const
{
	return fUboCount;
}

const std::vector<Texture*>& Frame::GetTextureVec() const
{
	return pTextureVec;
}

VkBuffer Frame::GetFrameUniformBuffer() const
{
	return frameUniformBuffer;
}

const FrameUniformBufferObject& Frame::GetFrameUniformBufferObject() const
{
	return fUBO;
}

VkDescriptorSet* Frame::GetFrameDescriptorSetPtr()
{
	return &frameDescriptorSet;
}

VkDescriptorSetLayout Frame::GetFrameDescriptorSetLayout() const
{
	return frameDescriptorSetLayout;
}

void Frame::AddTexture(Texture* pTexture)
{
	pTextureVec.push_back(pTexture);
}

void Frame::UpdateFrameUniformBuffer()
{
	void* data;
	vkMapMemory(pRenderer->GetDevice(), frameUniformBufferMemory, 0, sizeof(fUBO), 0, &data);
	memcpy(data, &fUBO, sizeof(fUBO));
	vkUnmapMemory(pRenderer->GetDevice(), frameUniformBufferMemory);
}

void Frame::CreateFrameUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(FrameUniformBufferObject);

	pRenderer->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		frameUniformBuffer,
		frameUniformBufferMemory);

	//initial update
	UpdateFrameUniformBuffer();
}
