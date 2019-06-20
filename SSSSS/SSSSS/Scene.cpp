#include "Scene.h"

Scene::Scene(const std::string& _name) : 
	pRenderer(nullptr), name(_name)
{
}

Scene::~Scene()
{
	CleanUp();
}

void Scene::InitScene(Renderer* _pRenderer, VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("scene " + name + " : pRenderer is null!");
	}
	pRenderer = _pRenderer;

	//must init texture before init pass
	for (auto pCamera : pCameraVec)
	{
		pCamera->InitCamera(pRenderer);
	}

	for (auto pShader : pShaderVec)
	{
		pShader->InitShader(pRenderer);
	}

	for (auto pTexture : pTextureVec)
	{
		pTexture->InitTexture(pRenderer);
	}

	for (auto pMesh : pMeshVec)
	{
		pMesh->InitMesh(pRenderer, descriptorPool, descriptorSetLayouts[static_cast<uint32_t>(UNIFORM_SLOT::OBJECT)]);
	}

	for (auto pPass : pPassVec)
	{
		pPass->InitPass(pRenderer, descriptorPool, descriptorSetLayouts[static_cast<uint32_t>(UNIFORM_SLOT::PASS)]);
	}

	CreateSceneUniformBuffer();
	pRenderer->CreateDescriptorSet(
		sceneDescriptorSet, 
		descriptorPool, 
		descriptorSetLayouts[static_cast<uint32_t>(UNIFORM_SLOT::SCENE)]);
	
	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(sceneUniformBuffer, sizeof(sUBO), { sceneDescriptorSet }, UniformSlotData[static_cast<uint32_t>(UNIFORM_SLOT::SCENE)].uboBindingOffset + 0);
}

void Scene::CleanUp()
{
	for (auto pShader : pShaderVec)
	{
		pShader->CleanUp();
	}

	for (auto pMesh : pMeshVec)
	{
		pMesh->CleanUp();
	}

	for (auto pTexture : pTextureVec)
	{
		pTexture->CleanUp();
	}

	for (auto pCamera : pCameraVec)
	{
		pCamera->CleanUp();
	}

	for (auto pPass : pPassVec)
	{
		pPass->CleanUp();
	}

	if (pRenderer != nullptr)
	{
		vkDestroyBuffer(pRenderer->GetDevice(), sceneUniformBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), sceneUniformBufferMemory, nullptr);
		pRenderer = nullptr;
	}
}

const std::vector<Pass*>& Scene::GetPassVec() const
{
	return pPassVec;
}

const std::vector<Mesh*>& Scene::GetMeshVec() const
{
	return pMeshVec;
}

const std::vector<Texture*>& Scene::GetTextureVec() const
{
	return pTextureVec;
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

void Scene::AddShader(Shader* pShader)
{
	if(pShader!=nullptr)
		pShaderVec.push_back(pShader);
}

void Scene::AddMesh(Mesh* pMesh)
{
	if (pMesh != nullptr)
		pMeshVec.push_back(pMesh);
}

void Scene::AddCamera(Camera* pCamera)
{
	if (pCamera != nullptr)
		pCameraVec.push_back(pCamera);
}

void Scene::AddTexture(Texture* pTexture)
{
	if (pTexture != nullptr)
		pTextureVec.push_back(pTexture);
}

void Scene::AddPass(Pass* pPass)
{
	if (pPass != nullptr)
		pPassVec.push_back(pPass);
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