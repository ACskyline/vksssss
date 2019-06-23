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
	pMeshVec.push_back(_pMesh);
}

void Pass::AddTexture(Texture* _pTexture)
{
	pTextureVec.push_back(_pTexture);
}

void Pass::AddRenderTexture(RenderTexture* pRenderTexture)
{
	pRenderTextureVec.push_back(pRenderTexture);
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

int Pass::GetRenderTextureCount() const
{
	return static_cast<int>(pRenderTextureVec.size());
}

bool Pass::HasRenderTexture() const
{
	return pRenderTextureVec.size() > 0;
}

VkSampleCountFlagBits Pass::GetMsaaSamples() const
{
	if (pRenderTextureVec.size() <= 0)
	{
		return VK_SAMPLE_COUNT_1_BIT;
	}
	else
	{
		for (auto pRenderTexture : pRenderTextureVec)
		{
			if (pRenderTexture->SupportMsaa())
				return pRenderTexture->GetMsaaSamples();
		}
	}
	return VK_SAMPLE_COUNT_1_BIT;
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

	//renderPass, extent and framebuffer
	if (pRenderTextureVec.size() > 0)
	{
		std::vector<VkAttachmentDescription> colorAttachments(pRenderTextureVec.size());
		std::vector<VkAttachmentDescription> depthAttachments;
		std::vector<VkAttachmentDescription> preResolveAttachments;
		std::vector<VkImageView> colorViews(pRenderTextureVec.size());
		std::vector<VkImageView> depthViews;
		std::vector<VkImageView> preResolveViews;
		int widthMax = 0;
		int heightMax = 0;
		for (int i = 0; i < pRenderTextureVec.size(); i++)
		{
			if (widthMax < pRenderTextureVec[i]->GetWidth()) widthMax = pRenderTextureVec[i]->GetWidth();
			if (heightMax < pRenderTextureVec[i]->GetHeight()) heightMax = pRenderTextureVec[i]->GetHeight();

			colorAttachments[i] = pRenderTextureVec[i]->GetColorAttachment();
			colorViews[i] = pRenderTextureVec[i]->GetColorImageView();

			if (pRenderTextureVec[i]->SupportDepth())
			{
				depthAttachments.push_back(pRenderTextureVec[i]->GetDepthAttachment());
				depthViews.push_back(pRenderTextureVec[i]->GetDepthImageView());
			}

			if (pRenderTextureVec[i]->SupportMsaa())
			{
				preResolveAttachments.push_back(pRenderTextureVec[i]->GetPreResolveAttachment());
				preResolveViews.push_back(pRenderTextureVec[i]->GetPreResolveImageView());
			}
		}
		extent.height = heightMax;
		extent.width = widthMax;
		pRenderer->CreateRenderPass(renderPass, preResolveAttachments, depthAttachments, colorAttachments);
		pRenderer->CreateFramebuffer(framebuffer, preResolveViews, depthViews, colorViews, renderPass, widthMax, heightMax);
	}
	else
	{
		//do nothing
		//renderPass, framebuffer and extent are all invalid
	}
}

void Pass::RecreateFramebuffer()
{
	//release the last one
	vkDestroyFramebuffer(pRenderer->GetDevice(), framebuffer, nullptr);

	if (pRenderTextureVec.size() > 0)
	{
		//create a new one
		std::vector<VkAttachmentDescription> colorAttachments(pRenderTextureVec.size());
		std::vector<VkAttachmentDescription> depthAttachments;
		std::vector<VkAttachmentDescription> preResolveAttachments;
		std::vector<VkImageView> colorViews(pRenderTextureVec.size());
		std::vector<VkImageView> depthViews;
		std::vector<VkImageView> preResolveViews;
		int widthMax = 0;
		int heightMax = 0;
		for (int i = 0; i < pRenderTextureVec.size(); i++)
		{
			if (widthMax < pRenderTextureVec[i]->GetWidth()) widthMax = pRenderTextureVec[i]->GetWidth();
			if (heightMax < pRenderTextureVec[i]->GetHeight()) heightMax = pRenderTextureVec[i]->GetHeight();

			colorAttachments[i] = pRenderTextureVec[i]->GetColorAttachment();
			colorViews[i] = pRenderTextureVec[i]->GetColorImageView();

			if (pRenderTextureVec[i]->SupportDepth())
			{
				depthAttachments.push_back(pRenderTextureVec[i]->GetDepthAttachment());
				depthViews.push_back(pRenderTextureVec[i]->GetDepthImageView());
			}

			if (pRenderTextureVec[i]->SupportMsaa())
			{
				preResolveAttachments.push_back(pRenderTextureVec[i]->GetPreResolveAttachment());
				preResolveViews.push_back(pRenderTextureVec[i]->GetPreResolveImageView());
			}
		}
		extent.height = heightMax;
		extent.width = widthMax;
		pRenderer->CreateRenderPass(renderPass, preResolveAttachments, depthAttachments, colorAttachments);
		pRenderer->CreateFramebuffer(framebuffer, preResolveViews, depthViews, colorViews, renderPass, widthMax, heightMax);
	}
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
