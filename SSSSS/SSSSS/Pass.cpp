#include "Pass.h"

#include "Camera.h"
#include "Texture.h"
#include "Mesh.h"
#include "Renderer.h"

Pass::Pass(const std::string& _name, 
	bool _clearColor, 
	bool _clearDepth, 
	bool _clearStencil,
	bool _enableDepthTest,
	bool _enableDepthWrite,
	bool _enableStencil,
	VkCompareOp _stencilCompareOp,
	VkStencilOp _depthFailOp,
	VkStencilOp _stencilFailOp,
	VkStencilOp _stencilPassOp,
	uint32_t _stencilReference) :
	pRenderer(nullptr), pScene(nullptr), pCamera(nullptr), name(_name), clearColor(_clearColor), clearDepth(_clearDepth), clearStencil(_clearStencil),
	enableDepthTest(_enableDepthTest),
	enableDepthWrite(_enableDepthWrite),
	enableStencil(_enableStencil),
	stencilCompareOp(_stencilCompareOp),
	depthFailOp(_depthFailOp),
	stencilFailOp(_stencilFailOp),
	stencilPassOp(_stencilPassOp),
	stencilReference(_stencilReference)
{
	for (auto& pShader : pShaderArr)
		pShader = nullptr;
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

bool Pass::IsClearColorEnabled() const
{
	return clearColor;
}

bool Pass::IsClearDepthStencilEnabled() const
{
	return clearDepth || clearStencil;
}

bool Pass::IsDepthTestEnabled() const
{
	return enableDepthTest;
}

bool Pass::IsDepthWriteEnabled() const
{
	return enableDepthWrite;
}

bool Pass::IsStencilEnabled() const
{
	return enableStencil;
}

VkCompareOp Pass::GetStencilCompareOp() const
{
	return stencilCompareOp;
}

VkStencilOp Pass::GetDepthFailOp() const
{
	return depthFailOp;
}

VkStencilOp Pass::GetStencilPassOp() const
{
	return stencilPassOp;
}

VkStencilOp Pass::GetStencilFailOp() const
{
	return stencilFailOp;
}

uint32_t Pass::GetStencilReference() const
{
	return stencilReference;
}

int Pass::GetColorRenderTextureCount() const
{
	int n = 0;
	for (auto& pRT : pRenderTextureVec)
	{
		if (pRT->SupportColor())
			n++;
	}
	return n;
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

uint32_t Pass::GetTextureCount() const
{
	return static_cast<uint32_t>(pTextureVec.size());
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
		0,
		pUboCount,//only 1 pUBO
		pUboCount,//only 1 pUBO, so offset is 1
		static_cast<uint32_t>(pTextureVec.size()));
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
		std::vector<VkAttachmentDescription> colorAttachments;
		std::vector<VkAttachmentDescription> depthAttachments;
		std::vector<VkAttachmentDescription> preResolveAttachments;
		std::vector<VkImageView> colorViews;
		std::vector<VkImageView> depthViews;
		std::vector<VkImageView> preResolveViews;
		int widthMax = 0;
		int heightMax = 0;
		for (int i = 0; i < pRenderTextureVec.size(); i++)
		{
			if (widthMax < pRenderTextureVec[i]->GetWidth()) widthMax = pRenderTextureVec[i]->GetWidth();
			if (heightMax < pRenderTextureVec[i]->GetHeight()) heightMax = pRenderTextureVec[i]->GetHeight();

			if (pRenderTextureVec[i]->SupportColor())
			{
				colorAttachments.push_back(pRenderTextureVec[i]->GetColorAttachment(clearColor));
				colorViews.push_back(pRenderTextureVec[i]->GetColorImageView());
			}

			if (pRenderTextureVec[i]->SupportDepthStencil())
			{
				depthAttachments.push_back(pRenderTextureVec[i]->GetDepthStencilAttachment(clearDepth, clearStencil));
				depthViews.push_back(pRenderTextureVec[i]->GetDepthStencilImageView());
			}

			if (pRenderTextureVec[i]->SupportMsaa())
			{
				preResolveAttachments.push_back(pRenderTextureVec[i]->GetPreResolveAttachment(clearColor));
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

void Pass::UpdatePassUniformBuffer()
{
	UpdatePassUniformBuffer(pCamera);
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

void Pass::UpdatePassUniformBuffer(Camera* _pCamera)
{
	pUBO.view = _pCamera->GetViewMatrix();
	pUBO.proj = _pCamera->GetProjectionMatrix();
	pUBO.cameraPosition = glm::vec4(_pCamera->position, 1.0);
	pUBO.passNum = 100;
	pUBO.widthTex = pTextureVec.size() > 0 ? pTextureVec[0]->GetWidth() : 0;
	pUBO.heightTex = pTextureVec.size() > 0 ? pTextureVec[0]->GetHeight() : 0;

	void* data;
	vkMapMemory(pRenderer->GetDevice(), passUniformBufferMemory, 0, sizeof(pUBO), 0, &data);
	memcpy(data, &pUBO, sizeof(pUBO));
	vkUnmapMemory(pRenderer->GetDevice(), passUniformBufferMemory);
}
