#pragma once
#include "Mesh.h"
#include "Camera.h"
#include "Shader.h"
#include "Texture.h"
#include "Pass.h"

class Pass;
class Mesh;
class Shader;

class Scene
{
public:
	Scene(const std::string& _name);
	~Scene();

	const std::vector<Pass*>& GetPassVec() const;
	const std::vector<Mesh*>& GetMeshVec() const;
	const std::vector<Texture*>& GetTextureVec() const;
	VkBuffer GetSceneUniformBuffer() const;
	const SceneUniformBufferObject& GetSceneUniformBufferObject() const;
	VkDescriptorSet* GetSceneDescriptorSetPtr();

	void AddPass(Pass* pPass);
	void AddMesh(Mesh* pMesh);
	void AddTexture(Texture* pTexture);
	void AddShader(Shader* pShader);
	void AddCamera(Camera* pCamera);

	void InitScene(Renderer* _pRenderer, VkDescriptorPool descriptorPool, const std::vector<VkDescriptorSetLayout>& descriptorSetLayouts);

	void CleanUp();

private:
	Renderer* pRenderer;
	std::string name;

	//asset containers
	std::vector<Pass*> pPassVec;
	std::vector<Mesh*> pMeshVec;
	std::vector<Texture*> pTextureVec;
	std::vector<Shader*> pShaderVec;
	std::vector<Camera*> pCameraVec;

	//scene uniform
	SceneUniformBufferObject sUBO;
	VkBuffer sceneUniformBuffer;
	VkDeviceMemory sceneUniformBufferMemory;
	VkDescriptorSet sceneDescriptorSet;

	//vulkan functions
	void UpdateSceneUniformBuffer();
	void CreateSceneUniformBuffer();
};
