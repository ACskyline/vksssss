#pragma once

#include "GlobalInclude.h"

class Renderer;
class Texture;

class Mesh
{
public:
	enum class MeshType { Square, FullScreenQuad, File, Count };

	Mesh(const std::string& _name, MeshType _type, const glm::vec3& _position, const glm::vec3& _rotation, const glm::vec3& _scale);
	~Mesh();

	const ObjectUniformBufferObject& GetObjectUniformBufferObject() const;
	VkBuffer GetObjectUniformBuffer() const;
	VkBuffer GetVertexBuffer() const;
	VkBuffer GetIndexBuffer() const;
	VkDescriptorSet* GetObjectDescriptorSetPtr(int frame);
	int GetObjectDescriptorSetCount() const;
	const std::vector<uint32_t>& GetIndexVec() const;
	uint32_t GetTextureCount() const;
	uint32_t GetUboCount() const;
	VkDescriptorSetLayout GetObjectDescriptorSetLayout() const;
	void UpdateObjectUniformBuffer(int frame);

	void InitMesh(Renderer* _pRenderer, VkDescriptorPool descriptorPool);
	void CleanUp();

	glm::vec3 position;
	glm::vec3 scale;
	glm::vec3 rotation;
private:
	struct Point
	{
		uint32_t VI;
		uint32_t TI;
		uint32_t NI;
	};

	Renderer* pRenderer;
	std::string name;
	const uint32_t oUboCount = 1;
	MeshType type;

	//assets
	std::vector<Texture*> pTextureVec;

	//mesh properties
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	//object uniform
	ObjectUniformBufferObject oUBO;
	VkBuffer objectUniformBuffer;
	VkDeviceMemory objectUniformBufferMemory;
	std::vector<VkDescriptorSet> objectDescriptorSetVec;
	VkDescriptorSetLayout objectDescriptorSetLayout;

	//vulkan functions
	void CreateObjectUniformBuffer(int frameCount);
	void CreateVertexBuffer();
	void CreateIndexBuffer();

	void InitSquare();
	//this full screen quad is defined in NDC, it does multiply view matrix, so the winding is the opposite
	void InitFullScreenQuad();
	void InitFromFile(const std::string& fileName);

	//mesh functions
	void LoadObjMesh(const std::string& fileName);
	void ParseObjFace(std::stringstream& ss, std::vector<Point>& tempVecPoint);
	void AssembleObjMesh(
		const std::vector<glm::vec3>& vecPos,
		const std::vector<glm::vec2>& vecUV,
		const std::vector<glm::vec3>& vecNor,
		const std::vector<Point>& vecPoint);
};
