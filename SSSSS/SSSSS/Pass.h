#pragma once

#include "GlobalInclude.h"
#include "Shader.h"

class Renderer;
class Scene;
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
	void AddRenderTexture(RenderTexture* pRenderTexture);
	void SetCamera(Camera* pCamera);
	void AddShader(Shader* pShader);
	void SetScene(Scene* _pScene);

	int GetRenderTextureCount() const;
	bool HasRenderTexture() const;
	VkSampleCountFlagBits GetMsaaSamples() const;
	uint32_t GetUboCount() const;
	Shader* GetShader(Shader::ShaderType type) const;
	const std::vector<Mesh*>& GetMeshVec() const;
	const std::vector<Texture*>& GetTextureVec() const;
	const PassUniformBufferObject& GetPassUniformBufferObject() const;
	VkDescriptorSet* GetPassDescriptorSetPtr();
	VkBuffer GetPassUniformBuffer() const;
	Camera* GetCamera() const;
	Scene* GetScene() const;
	VkRenderPass GetRenderPass() const;
	VkFramebuffer GetFramebuffer() const;
	VkExtent2D GetExtent() const;
	VkDescriptorSetLayout GetPassDescriptorSetLayout() const;
	//# This is an error-tolerant function, all mesh will have the same amount of texture slot, but some may use less.
	//# You won't have any debug layer error with this, but the texture with higher slot number will remain the same for those who use less slots.
	VkDescriptorSetLayout GetLargestObjectDescriptorSetLayout() const;
	//# This is a conservative function, all mesh will have the same number of texture slot, but textures with higher slot number will be omitted.
	//# You will have a debug layer error when you want to bind more textures than the descriptor set layout allows.
	VkDescriptorSetLayout GetSmallestObjectDescriptorSetLayout() const;

	//# If no render texture is present, swap render pass & extent will be used.
	void InitPass(
		Renderer* _pRenderer, 
		VkDescriptorPool descriptorPool);

	//# Can't recreate render pass because a pipeline is fixed to the render pass.
	//# If we want to reuse the pipeline, the render pass must remain the same.
	//# This means if we want to dynamically change render target, we can't change the format and total number of render textures, but only which render textures we want to render to.
	void RecreateFramebuffer();
	
	//# This function does not check whether the slot is valid nor whether the format is the same.
	void ChangeRenderTexture(uint32_t slot, RenderTexture* pRenderTexture);

	void CleanUp();

private:
	Renderer* pRenderer;
	Scene* pScene;
	std::string name;
	const uint32_t pUboCount = 1;

	//asset containers
	std::vector<Mesh*> pMeshVec;
	std::array<Shader*, static_cast<int>(Shader::ShaderType::Count)> pShaderArr;
	std::vector<Texture*> pTextureVec;
	std::vector<RenderTexture*> pRenderTextureVec;
	Camera* pCamera;

	//pass uniform
	PassUniformBufferObject pUBO;
	VkBuffer passUniformBuffer;
	VkDeviceMemory passUniformBufferMemory;
	VkDescriptorSet passDescriptorSet;
	VkDescriptorSetLayout passDescriptorSetLayout;

	//render texture property
	VkRenderPass renderPass;
	VkFramebuffer framebuffer;
	VkExtent2D extent;

	//vulkan functions
	void UpdatePassUniformBuffer();
	void CreatePassUniformBuffer();
};
