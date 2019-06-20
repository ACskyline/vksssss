#pragma once
#include "Renderer.h"
#include "GlobalInclude.h"

class Renderer;

class Mesh
{
public:
	enum MeshType { Square, Count };

	Mesh(const std::string& _name, MeshType _type, const glm::vec3& _position, const glm::vec3& _rotation, const glm::vec3& _scale);
	~Mesh();

	const ObjectUniformBufferObject& GetObjectUniformBufferObject() const;
	VkBuffer GetObjectUniformBuffer() const;
	VkBuffer GetVertexBuffer() const;
	VkBuffer GetIndexBuffer() const;
	VkDescriptorSet* GetObjectDescriptorSetPtr();
	const std::vector<uint16_t>& GetIndexVec() const;

	void InitMesh(Renderer* _pRenderer, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorLayout);
	void CleanUp();

private:
	Renderer* pRenderer;
	std::string name;
	MeshType type;
	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;

	//vulkan variables
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

	//vulkan functions
	void UpdateObjectUniformBuffer();
	void CreateObjectUniformBuffer();
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void InitSquare();
};
