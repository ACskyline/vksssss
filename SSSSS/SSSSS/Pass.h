#pragma once
#include "Mesh.h"
#include "Camera.h"
#include "Texture.h"

class Mesh;
class Camera;
class Texture;
class RenderTexture;

class Pass
{
public:
	Pass(const std::string& _name);
	~Pass();

	void AddMesh(Mesh* pMesh);
	void AddTexture(Texture* pTexture);
	void SetCamera(Camera* pCamera);

	const std::vector<Mesh*>& GetMeshVec() const;
	const std::vector<Texture*>& GetTextureVec() const;
	const PassUniformBufferObject& GetPassUniformBufferObject() const;
	VkDescriptorSet* GetPassDescriptorSetPtr();
	VkBuffer GetPassUniformBuffer() const;
	Camera* GetCamera() const;
	VkRenderPass GetRenderPass() const;
	VkFramebuffer GetFramebuffer() const;
	VkExtent2D GetExtent() const;

	void InitPass(Renderer* _pRenderer, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorLayout);

	void CleanUp();

private:
	Renderer* pRenderer;
	std::string name;

	//asset containers
	std::vector<Mesh*> pMeshVec;
	std::vector<Texture*> pTextureVec;
	std::vector<RenderTexture*> pRenderTextureVec;
	Camera* pCamera;

	//pass uniform
	PassUniformBufferObject pUBO;
	VkBuffer passUniformBuffer;
	VkDeviceMemory passUniformBufferMemory;
	VkDescriptorSet passDescriptorSet;

	//framebuffer
	VkFramebuffer framebuffer;

	//render pass
	VkRenderPass renderPass;

	//extent
	VkExtent2D extent;

	//vulkan functions
	void UpdatePassUniformBuffer();
	void CreatePassUniformBuffer();
};
