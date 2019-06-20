#include "Mesh.h"

Mesh::Mesh(const std::string& _name, MeshType _type, const glm::vec3& _position, const glm::vec3& _rotation, const glm::vec3& _scale) :
	pRenderer(nullptr), name(_name), type(_type), position(_position), rotation(_rotation), scale(_scale)
{
}

Mesh::~Mesh()
{
	CleanUp();
}

const ObjectUniformBufferObject& Mesh::GetObjectUniformBufferObject() const
{
	return oUBO;
}

VkBuffer Mesh::GetObjectUniformBuffer() const
{
	return objectUniformBuffer;
}

VkBuffer Mesh::GetVertexBuffer() const
{
	return vertexBuffer;
}

VkBuffer Mesh::GetIndexBuffer() const
{
	return indexBuffer;
}

VkDescriptorSet* Mesh::GetObjectDescriptorSetPtr()
{
	return &objectDescriptorSet;
}

const std::vector<uint16_t>& Mesh::GetIndexVec() const
{
	return indices;
}

void Mesh::InitMesh(Renderer* _pRenderer, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout)
{
	if (_pRenderer == nullptr)
	{
		throw std::runtime_error("mesh " + name + " : pRenderer is null!");
	}

	pRenderer = _pRenderer;
	if (type == MeshType::Square)
	{
		InitSquare();
	}

	CreateObjectUniformBuffer();
	CreateVertexBuffer();
	CreateIndexBuffer();
	pRenderer->CreateDescriptorSet(
		objectDescriptorSet,
		descriptorPool,
		descriptorSetLayout);

	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(objectUniformBuffer, sizeof(oUBO), { objectDescriptorSet }, UniformSlotData[static_cast<uint32_t>(UNIFORM_SLOT::OBJECT)].uboBindingOffset + 0);
}

void Mesh::UpdateObjectUniformBuffer()
{
	oUBO.model = glm::translate(glm::mat4(1.0f), position) *
		glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *
		glm::scale(glm::mat4(1.0f), scale);

	void* data;
	vkMapMemory(pRenderer->GetDevice(), objectUniformBufferMemory, 0, sizeof(oUBO), 0, &data);
	memcpy(data, &oUBO, sizeof(oUBO));
	vkUnmapMemory(pRenderer->GetDevice(), objectUniformBufferMemory);
}

void Mesh::CreateObjectUniformBuffer()
{
	VkDeviceSize bufferSize = sizeof(ObjectUniformBufferObject);

	pRenderer->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		objectUniformBuffer,
		objectUniformBufferMemory);

	//initial update
	UpdateObjectUniformBuffer();
}

void Mesh::CreateVertexBuffer() {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	pRenderer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(pRenderer->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(pRenderer->GetDevice(), stagingBufferMemory);

	pRenderer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

	pRenderer->CopyBuffer(pRenderer->GetGraphicsCommandPool(), stagingBuffer, vertexBuffer, bufferSize);

	vkDestroyBuffer(pRenderer->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pRenderer->GetDevice(), stagingBufferMemory, nullptr);
}

void Mesh::CreateIndexBuffer() {
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	pRenderer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(pRenderer->GetDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(pRenderer->GetDevice(), stagingBufferMemory);

	pRenderer->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

	pRenderer->CopyBuffer(pRenderer->GetGraphicsCommandPool(), stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(pRenderer->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pRenderer->GetDevice(), stagingBufferMemory, nullptr);
}

void Mesh::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyBuffer(pRenderer->GetDevice(), objectUniformBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), objectUniformBufferMemory, nullptr);

		vkDestroyBuffer(pRenderer->GetDevice(), indexBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), indexBufferMemory, nullptr);

		vkDestroyBuffer(pRenderer->GetDevice(), vertexBuffer, nullptr);
		vkFreeMemory(pRenderer->GetDevice(), vertexBufferMemory, nullptr);

		pRenderer = nullptr;
	}
}

void Mesh::InitSquare()
{
	vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};

	indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};
}
