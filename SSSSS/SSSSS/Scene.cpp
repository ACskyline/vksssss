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
	CreateSceneUniformBuffer(_pRenderer->frameCount);
	pRenderer->CreateDescriptorSetLayoutTextureArray(
		sceneDescriptorSetLayout,
		0,
		sUboCount,//only 1 sUBO
		sUboCount,//only 1 sUBO, so offset is 1
		{ static_cast<uint32_t>(pTextureVec.size()),//texture arrays instead of textures
		  static_cast<uint32_t>(pTextureVec2.size()) });//this is for aliasing of sampler2D and sampler2DShadow
	sceneDescriptorSetVec.resize(_pRenderer->frameCount);
	pRenderer->CreateDescriptorSets(
		sceneDescriptorSetVec,
		descriptorPool,
		sceneDescriptorSetLayout);

	VkDeviceSize uboSize = sizeof(sUBO);
	for (int i = 0; i < sceneDescriptorSetVec.size(); i++)
	{
		//bind ubo
		VkDeviceSize uboOffset = _pRenderer->GetAlignedUboOffset(uboSize, i);
		pRenderer->BindUniformBufferToDescriptorSets(sceneUniformBuffer, uboOffset, uboSize, { sceneDescriptorSetVec[i] }, 0);//only 1 sUBO

		//bind texture array
		//for (int j = 0; j < pTextureVec.size(); i++)
		//{
		//	pRenderer->BindTextureToDescriptorSets(pTextureVec[j]->GetTextureImageView(), pTextureVec[j]->GetSampler(), { sceneDescriptorSet }, sUboCount + 0, j);//only 1 sUBO, so offset is 1. since this is texture array, elementOffset is not 0 anymore
		//}

		//another way to bind texture array
		pRenderer->BindTextureArrayToDescriptorSets(pTextureVec, { sceneDescriptorSetVec[i] }, sUboCount + 0);
		//an extra array to support alias of sampler2D and sampler2DShadow
		pRenderer->BindTextureArrayToDescriptorSets(pTextureVec2, { sceneDescriptorSetVec[i] }, sUboCount + 1);
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
	return static_cast<uint32_t>(pTextureVec.size() * 2);//times 2 for alias of sampler2D and sampler2DShadow
}

VkBuffer Scene::GetSceneUniformBuffer() const
{
	return sceneUniformBuffer;
}

const SceneUniformBufferObject& Scene::GetSceneUniformBufferObject() const
{
	return sUBO;
}

VkDescriptorSet* Scene::GetSceneDescriptorSetPtr(int frame)
{
	return sceneDescriptorSetVec.data() + frame;
}

int Scene::GetSceneDescriptorSetCount() const
{
	return sUboCount;
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
		pTextureVec2.push_back(pLight->GetRenderTexturePtr2());
	}
}

void Scene::AddPass(Pass* pPass)
{
	pPassVec.push_back(pPass);
	pPass->SetScene(this);
}

void Scene::UpdateSceneUniformBuffer(int frame)
{
	uint32_t lightCount = static_cast<uint32_t>(pLightVec.size());
	if (lightCount > MAX_LIGHTS_PER_SCENE)
		lightCount = MAX_LIGHTS_PER_SCENE;
	sUBO.lightCount = lightCount;

	for (uint32_t i = 0; i < lightCount; i++)
	{
		sUBO.lightArr[i].view = pLightVec[i]->GetViewMatrix();
		sUBO.lightArr[i].proj = pLightVec[i]->GetProjectionMatrix();
		sUBO.lightArr[i].viewInv = glm::inverse(sUBO.lightArr[i].view);
		sUBO.lightArr[i].projInv = glm::inverse(sUBO.lightArr[i].proj);
		sUBO.lightArr[i].color = glm::vec4(pLightVec[i]->GetColor(), 1);
		sUBO.lightArr[i].position = glm::vec4(pLightVec[i]->GetPosition(), 1);
		sUBO.lightArr[i].textureIndex = pLightVec[i]->GetTextureIndex();
		sUBO.lightArr[i].near = pLightVec[i]->GetNear();
		sUBO.lightArr[i].far = pLightVec[i]->GetFar();
	}

	void* data;
	VkDeviceSize size = sizeof(sUBO);
	VkDeviceSize offset = pRenderer->GetAlignedUboOffset(size, frame);
	vkMapMemory(pRenderer->GetDevice(), sceneUniformBufferMemory, offset, size, 0, &data);
	memcpy(data, &sUBO, sizeof(sUBO));
	vkUnmapMemory(pRenderer->GetDevice(), sceneUniformBufferMemory);
}

void Scene::CreateSceneUniformBuffer(int frameCount)
{
	VkDeviceSize bufferSize = pRenderer->GetAlignedUboSize(sizeof(SceneUniformBufferObject)) * frameCount;

	pRenderer->CreateBuffer(bufferSize, 
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
		sceneUniformBuffer, 
		sceneUniformBufferMemory);

	for (int i = 0; i < frameCount; i++)
	{
		//initial update
		UpdateSceneUniformBuffer(i);
	}
}