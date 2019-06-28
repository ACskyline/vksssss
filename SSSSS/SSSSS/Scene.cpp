#include "Scene.h"

#include "Renderer.h"
#include "Texture.h"
#include "Pass.h"

Scene::Scene(const std::string& _name) : 
	pRenderer(nullptr), name(_name)
{
}

Scene::~Scene()
{
	CleanUp();
}

void Scene::InitScene(Renderer* _pRenderer, VkDescriptorPool descriptorPool)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("scene " + name + " : pRenderer is null!");
	}
	pRenderer = _pRenderer;

	//create uniform resources
	CreateSceneUniformBuffer();
	pRenderer->CreateDescriptorSetLayout(
		sceneDescriptorSetLayout,
		sUboCount,//only 1 sUBO
		0,
		static_cast<uint32_t>(pTextureVec.size()),
		sUboCount);//only 1 sUBO, so offset is 1
	pRenderer->CreateDescriptorSet(
		sceneDescriptorSet,
		descriptorPool,
		sceneDescriptorSetLayout);
	
	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(sceneUniformBuffer, sizeof(sUBO), { sceneDescriptorSet }, 0);//only 1 sUBO

	//bind texture
	for (int i = 0; i < pTextureVec.size(); i++)
	{
		pRenderer->BindTextureToDescriptorSets(pTextureVec[i]->GetTextureImageView(), pTextureVec[i]->GetSampler(), { sceneDescriptorSet }, sUboCount + i);//only 1 sUBO, so offset is 1
	}
}

void Scene::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyDescriptorSetLayout(pRenderer->GetDevice(), sceneDescriptorSetLayout, nullptr);
		vkDestroyBuffer(pRenderer->GetDevice(), sceneUniformBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), sceneUniformBufferMemory, nullptr);
		pRenderer = nullptr;
	}
}

uint32_t Scene::GetUboCount() const
{
	return sUboCount;
}

const std::vector<Pass*>& Scene::GetPassVec() const
{
	return pPassVec;
}

uint32_t Scene::GetTextureCount() const
{
	return static_cast<uint32_t>(pTextureVec.size());
}

VkBuffer Scene::GetSceneUniformBuffer() const
{
	return sceneUniformBuffer;
}

const SceneUniformBufferObject& Scene::GetSceneUniformBufferObject() const
{
	return sUBO;
}

VkDescriptorSet* Scene::GetSceneDescriptorSetPtr()
{
	return &sceneDescriptorSet;
}

VkDescriptorSetLayout Scene::GetSceneDescriptorSetLayout() const
{
	return sceneDescriptorSetLayout;
}

void Scene::AddTexture(Texture* pTexture)
{
	pTextureVec.push_back(pTexture);
}

void Scene::AddPass(Pass* pPass)
{
	pPassVec.push_back(pPass);
	pPass->SetScene(this);
}

void Scene::UpdateSceneUniformBuffer()
{
	void* data;
	vkMapMemory(pRenderer->GetDevice(), sceneUniformBufferMemory, 0, sizeof(sUBO), 0, &data);
	memcpy(data, &sUBO, sizeof(sUBO));
	vkUnmapMemory(pRenderer->GetDevice(), sceneUniformBufferMemory);
}

void Scene::CreateSceneUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(SceneUniformBufferObject);

	pRenderer->CreateBuffer(bufferSize, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		sceneUniformBuffer, 
		sceneUniformBufferMemory);

	//initial update
	UpdateSceneUniformBuffer();
}