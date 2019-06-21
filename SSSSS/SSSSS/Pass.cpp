#include "Pass.h"

#include "Camera.h"
#include "Texture.h"
#include "Mesh.h"
#include "Renderer.h"

Pass::Pass(const std::string& _name) : 
	pRenderer(nullptr), pScene(nullptr), pCamera(nullptr), name(_name)
{
	for (auto& pShader : pShaderArr)
		pShader = nullptr;

	pUBO.passNum = 0;
	pUBO.proj = glm::mat4(1);
	pUBO.view = glm::mat4(1);
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

void Pass::SetScene(Scene* _pScene)
{
	pScene = _pScene;
}

void Pass::AddShader(Shader* pShader)
{
	pShaderArr[static_cast<int>(pShader->GetShaderType())] = pShader;
}

bool Pass::HasRenderTexture() const
{
	return pRenderTextureVec.size() > 0;
}

uint32_t Pass::GetUboCount() const
{
	return pUboCount;
}

Shader* Pass::GetShader(Shader::ShaderType type) const
{
	return pShaderArr[static_cast<int>(type)];
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

Scene* Pass::GetScene() const
{
	return pScene;
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

VkDescriptorSetLayout Pass::GetLargestObjectDescriptorSetLayout() const
{
	VkDescriptorSetLayout result = {};
	int textureCountMax = -1;

	for (auto pMesh : pMeshVec)
	{
		int textureCount = pMesh->GetTextureCount();
		if (textureCount > textureCountMax)
		{
			textureCountMax = textureCount;
			result = pMesh->GetObjectDescriptorSetLayout();
		}
	}

	return result;
}

VkDescriptorSetLayout Pass::GetSmallestObjectDescriptorSetLayout() const
{
	VkDescriptorSetLayout result = {};
	int textureCountMin = std::numeric_limits<int>::max();

	for (auto pMesh : pMeshVec)
	{
		int textureCount = pMesh->GetTextureCount();
		if (textureCount < textureCountMin)
		{
			textureCountMin = textureCount;
			result = pMesh->GetObjectDescriptorSetLayout();
		}
	}

	return result;
}

VkDescriptorSetLayout Pass::GetPassDescriptorSetLayout() const
{
	return passDescriptorSetLayout;
}

void Pass::InitPass(
	Renderer* _pRenderer,
	VkDescriptorPool descriptorPool)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("pass " + name + " : pRenderer is null!");
	}

	pRenderer = _pRenderer;
	//create uniform resources
	CreatePassUniformBuffer();
	pRenderer->CreateDescriptorSetLayout(
		passDescriptorSetLayout,
		pUboCount,//only 1 pUBO
		0,
		static_cast<uint32_t>(pTextureVec.size()),
		pUboCount);//only 1 pUBO, so offset is 1
	pRenderer->CreateDescriptorSet(
		passDescriptorSet,
		descriptorPool,
		passDescriptorSetLayout);

	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(passUniformBuffer, sizeof(pUBO), { passDescriptorSet }, 0);//only 1 pUBO

	//bind texture
	for (int i = 0; i < pTextureVec.size(); i++)
	{
		pRenderer->BindTextureToDescriptorSets(pTextureVec[i]->GetTextureImageView(), pTextureVec[i]->GetSampler(), { passDescriptorSet }, 1 + i);//only 1 pUBO, so offset is 1
	}

	//renderPass, extent and potentially framebuffer
	if (pRenderTextureVec.size() > 0)
	{
		std::vector<VkAttachmentDescription> colorAttachments(pRenderTextureVec.size());
		std::vector<VkAttachmentDescription> depthAttachments;
		std::vector<VkImageView> colorViews(pRenderTextureVec.size());
		std::vector<VkImageView> depthViews;
		int width = 0;
		int height = 0;
		for (int i = 0; i < pRenderTextureVec.size(); i++)
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
	else
	{
		//do nothing
		//renderPass, framebuffer and extent are all invalid
	}
}

void Pass::RecreateFramebuffer()
{
	vkDestroyFramebuffer(pRenderer->GetDevice(), framebuffer, nullptr);
	std::vector<VkImageView> colorViews(pRenderTextureVec.size());
	std::vector<VkImageView> depthViews;
	int width = 0;
	int height = 0;
	for (int i = 0; i < pRenderTextureVec.size(); i++)
	{
		if (width < pRenderTextureVec[i]->GetWidth()) width = pRenderTextureVec[i]->GetWidth();
		if (height < pRenderTextureVec[i]->GetHeight()) height = pRenderTextureVec[i]->GetHeight();

		colorViews[i] = pRenderTextureVec[i]->GetColorImageView();
		if (pRenderTextureVec[i]->SupportDepth())
		{
			depthViews.push_back(pRenderTextureVec[i]->GetDepthImageView());
		}
	}
	extent.height = height;
	extent.width = width;
	pRenderer->CreateFramebuffer(framebuffer, colorViews, depthViews, renderPass, width, height);
}

void Pass::ChangeRenderTexture(uint32_t slot, RenderTexture* pRenderTexture)
{
	pRenderTextureVec[slot] = pRenderTexture;
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
		vkDestroyDescriptorSetLayout(pRenderer->GetDevice(), passDescriptorSetLayout, nullptr);
		vkDestroyFramebuffer(pRenderer->GetDevice(), framebuffer, nullptr);
		vkDestroyRenderPass(pRenderer->GetDevice(), renderPass, nullptr);
		vkDestroyBuffer(pRenderer->GetDevice(), passUniformBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), passUniformBufferMemory, nullptr);
		pRenderer = nullptr;
	}
}
