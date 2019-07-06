#include "Scene.h"

#include "Renderer.h"
#include "Texture.h"
#include "Pass.h"
#include "Light.h"

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
	pRenderer->CreateDescriptorSetLayoutTextureArray(
		sceneDescriptorSetLayout,
		0,
		sUboCount,//only 1 sUBO
		sUboCount,//only 1 sUBO, so offset is 1
		{ static_cast<uint32_t>(pTextureVec.size()) });//texture array instead of texture
	pRenderer->CreateDescriptorSet(
		sceneDescriptorSet,
		descriptorPool,
		sceneDescriptorSetLayout);

	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(sceneUniformBuffer, sizeof(sUBO), { sceneDescriptorSet }, 0);//only 1 sUBO

	//bind texture array
	//for (int i = 0; i < pTextureVec.size(); i++)
	//{
	//	pRenderer->BindTextureToDescriptorSets(pTextureVec[i]->GetTextureImageView(), pTextureVec[i]->GetSampler(), { sceneDescriptorSet }, sUboCount + 0, i);//only 1 sUBO, so offset is 1. since this is texture array, elementOffset is not 0 anymore
	//}

	//same effect
	pRenderer->BindTextureArrayToDescriptorSets(pTextureVec, { sceneDescriptorSet }, sUboCount + 0);
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

void Scene::AddLight(Light* pLight)
{
	pLight->SetScene(this);
	pLightVec.push_back(pLight);
	if (pLight->GetRenderTexturePtr() != nullptr)
	{
		pLight->SetTextureIndex(static_cast<int32_t>(pTextureVec.size()));
		pTextureVec.push_back(pLight->GetRenderTexturePtr());
	}
}

void Scene::AddPass(Pass* pPass)
{
	pPassVec.push_back(pPass);
	pPass->SetScene(this);
}

void Scene::UpdateSceneUniformBuffer()
{
	uint32_t lightCount = static_cast<uint32_t>(pLightVec.size());
	if (lightCount > MAX_LIGHTS_PER_SCENE)
		lightCount = MAX_LIGHTS_PER_SCENE;
	sUBO.lightCount = lightCount;

	for (uint32_t i = 0; i < lightCount; i++)
	{
		sUBO.lightArr[i].view = pLightVec[i]->GetViewMatrix();
		sUBO.lightArr[i].proj = pLightVec[i]->GetProjectionMatrix();
		sUBO.lightArr[i].color = glm::vec4(pLightVec[i]->GetColor(), 1);
		sUBO.lightArr[i].position = glm::vec4(pLightVec[i]->GetPosition(), 1);
		sUBO.lightArr[i].textureIndex = pLightVec[i]->GetTextureIndex();
	}

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