#include "Pass.h"

Pass::Pass(const std::string& _name) : 
	pRenderer(nullptr), pCamera(nullptr), name(_name)
{
}

Pass::~Pass()
{
	CleanUp();
}

void Pass::AddMesh(Mesh* _pMesh)
{
	if(_pMesh!=nullptr)
		pMeshVec.push_back(_pMesh);
}

void Pass::AddTexture(Texture* _pTexture)
{
	if (_pTexture != nullptr)
		pTextureVec.push_back(_pTexture);
}

void Pass::SetCamera(Camera* _pCamera)
{
	pCamera = _pCamera;
}

const std::vector<Mesh*>& Pass::GetMeshVec() const
{
	return pMeshVec;
}

const std::vector<Texture*>& Pass::GetTextureVec() const
{
	return pTextureVec;
}

Camera* Pass::GetCamera() const
{
	return pCamera;
}

VkBuffer Pass::GetPassUniformBuffer() const
{
	return passUniformBuffer;
}

const PassUniformBufferObject& Pass::GetPassUniformBufferObject() const
{
	return pUBO;
}

VkDescriptorSet* Pass::GetPassDescriptorSetPtr()
{
	return &passDescriptorSet;
}

VkRenderPass Pass::GetRenderPass() const
{
	return renderPass;
}

VkFramebuffer Pass::GetFramebuffer() const
{
	return framebuffer;
}

VkExtent2D Pass::GetExtent() const
{
	return extent;
}

void Pass::InitPass(Renderer* _pRenderer, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("pass " + name + " : pRenderer is null!");
	}

	pRenderer = _pRenderer;
	CreatePassUniformBuffer();
	pRenderer->CreateDescriptorSet(
		passDescriptorSet, 
		descriptorPool, 
		descriptorSetLayout);
	
	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(passUniformBuffer, sizeof(pUBO), { passDescriptorSet }, UniformSlotData[static_cast<uint32_t>(UNIFORM_SLOT::PASS)].uboBindingOffset + 0);
	
	//bind texture
	for (int i = 0; i < pTextureVec.size(); i++)
	{
		pRenderer->BindTextureToDescriptorSets(pTextureVec[i]->GetTextureImageView(), pTextureVec[i]->GetSampler(), { passDescriptorSet }, UniformSlotData[static_cast<uint32_t>(UNIFORM_SLOT::PASS)].textureBindingOffset + i);
	}

	//render texture, render pass and framebuffer
	std::vector<VkAttachmentDescription> colorAttachments(pRenderTextureVec.size());
	std::vector<VkAttachmentDescription> depthAttachments;
	std::vector<VkImageView> colorViews(pRenderTextureVec.size());
	std::vector<VkImageView> depthViews;
	int width = 0;
	int height = 0;
	for (int i= 0; i < pRenderTextureVec.size(); i++)
	{
		if (width < pRenderTextureVec[i]->GetWidth()) width = pRenderTextureVec[i]->GetWidth();
		if (height < pRenderTextureVec[i]->GetHeight()) height = pRenderTextureVec[i]->GetHeight();

		colorAttachments[i] = pRenderTextureVec[i]->GetColorAttachment();
		colorViews[i] = pRenderTextureVec[i]->GetColorImageView();
		if (pRenderTextureVec[i]->SupportDepth())
		{
			depthAttachments.push_back(pRenderTextureVec[i]->GetDepthAttachment());
			depthViews.push_back(pRenderTextureVec[i]->GetDepthImageView());
		}
	}
	extent.height = height;
	extent.width = width;
	pRenderer->CreateRenderPass(renderPass, colorAttachments, depthAttachments);
	pRenderer->CreateFramebuffer(framebuffer, colorViews, depthViews, renderPass, width, height);
}

void Pass::UpdatePassUniformBuffer()
{
	pUBO.view = pCamera->GetViewMatrix();
	pUBO.proj = pCamera->GetProjectionMatrix();
	pUBO.passNum = 100;

	void* data;
	vkMapMemory(pRenderer->GetDevice(), passUniformBufferMemory, 0, sizeof(pUBO), 0, &data);
	memcpy(data, &pUBO, sizeof(pUBO));
	vkUnmapMemory(pRenderer->GetDevice(), passUniformBufferMemory);
}

void Pass::CreatePassUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(PassUniformBufferObject);

	pRenderer->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		passUniformBuffer,
		passUniformBufferMemory);

	//initial update
	UpdatePassUniformBuffer();
}

void Pass::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyFramebuffer(pRenderer->GetDevice(), framebuffer, nullptr);
		vkDestroyRenderPass(pRenderer->GetDevice(), renderPass, nullptr);
		vkDestroyBuffer(pRenderer->GetDevice(), passUniformBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), passUniformBufferMemory, nullptr);
		pRenderer = nullptr;
	}
}
