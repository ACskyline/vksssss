#pragma once

#include "GlobalInclude.h"

class Renderer;
class Texture;

class Mesh
{
public:
	enum class MeshType { Square, Count };

	Mesh(const std::string& _name, MeshType _type, const glm::vec3& _position, const glm::vec3& _rotation, const glm::vec3& _scale);
	~Mesh();

	const ObjectUniformBufferObject& GetObjectUniformBufferObject() const;
	VkBuffer GetObjectUniformBuffer() const;
	VkBuffer GetVertexBuffer() const;
	VkBuffer GetIndexBuffer() const;
	VkDescriptorSet* GetObjectDescriptorSetPtr();
	const std::vector<uint16_t>& GetIndexVec() const;
	uint32_t GetTextureCount() const;
	uint32_t GetUboCount() const;
	VkDescriptorSetLayout GetObjectDescriptorSetLayout() const;

	void InitMesh(Renderer* _pRenderer, VkDescriptorPool descriptorPool);
	void CleanUp();

private:
	Renderer* pRenderer;
	std::string name;
	const uint32_t oUboCount = 1;
	MeshType type;
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;

	//assets
	std::vector<Texture*> pTextureVec;

	//mesh properties
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	//object uniform
	ObjectUniformBufferObject oUBO;
	VkBuffer objectUniformBuffer;
	VkDeviceMemory objectUniformBufferMemory;
	VkDescriptorSet objectDescriptorSet;
	VkDescriptorSetLayout objectDescriptorSetLayout;

	//vulkan functions
	void UpdateObjectUniformBuffer();
	void CreateObjectUniformBuffer();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void InitSquare();
};
