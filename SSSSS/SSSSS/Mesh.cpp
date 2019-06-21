#include "Mesh.h"

#include "Texture.h"
#include "Renderer.h"

Mesh::Mesh(const std::string& _name, MeshType _type, const glm::vec3& _position, const glm::vec3& _rotation, const glm::vec3& _scale) :
	pRenderer(nullptr), name(_name), type(_type), position(_position), rotation(_rotation), scale(_scale)
{
	oUBO.model = glm::mat4(1);
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

uint32_t Mesh::GetTextureCount() const
{
	return static_cast<uint32_t>(pTextureVec.size());
}

uint32_t Mesh::GetUboCount() const
{
	return oUboCount;
}

VkDescriptorSetLayout Mesh::GetObjectDescriptorSetLayout() const
{
	return objectDescriptorSetLayout;
}

void Mesh::InitMesh(Renderer* _pRenderer, VkDescriptorPool descriptorPool)
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

	//create vertex resources
	CreateVertexBuffer();
	CreateIndexBuffer();

	//create uniform resources
	CreateObjectUniformBuffer();
	pRenderer->CreateDescriptorSetLayout(
		objectDescriptorSetLayout,
		oUboCount,//only 1 oUBO
		0,
		static_cast<uint32_t>(pTextureVec.size()),
		oUboCount);//only 1 oUBO, so offset is 1
	pRenderer->CreateDescriptorSet(
		objectDescriptorSet,
		descriptorPool,
		objectDescriptorSetLayout);

	//bind ubo
	pRenderer->BindUniformBufferToDescriptorSets(objectUniformBuffer, sizeof(oUBO), { objectDescriptorSet }, 0);//only 1 oUBO

	//bind texture
	for (int i = 0; i < pTextureVec.size(); i++)
	{
		pRenderer->BindTextureToDescriptorSets(pTextureVec[i]->GetTextureImageView(), pTextureVec[i]->GetSampler(), { objectDescriptorSet }, oUboCount + i);//only 1 oUBO, so offset is 1
	}
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

	pRenderer->CopyBuffer(pRenderer->defaultCommandPool, stagingBuffer, vertexBuffer, bufferSize);

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

	pRenderer->CopyBuffer(pRenderer->defaultCommandPool, stagingBuffer, indexBuffer, bufferSize);

	vkDestroyBuffer(pRenderer->GetDevice(), stagingBuffer, nullptr);
	vkFreeMemory(pRenderer->GetDevice(), stagingBufferMemory, nullptr);
}

void Mesh::CleanUp()
{
	if (pRenderer != nullptr)
	{
		vkDestroyDescriptorSetLayout(pRenderer->GetDevice(), objectDescriptorSetLayout, nullptr);

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
