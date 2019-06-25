#include "Mesh.h"

#include <fstream>
#include <sstream>

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

const std::vector<uint32_t>& Mesh::GetIndexVec() const
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
	else if (type == MeshType::FullScreenQuad)
	{
		InitFullScreenQuad();
	}
	else if (type == MeshType::File)
	{
		InitFromFile(name);
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

	oUBO.modelInvTrans = glm::transpose(
		glm::scale(glm::mat4(1.0f), 1.f/scale) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-rotation.x), glm::vec3(1, 0, 0)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-rotation.y), glm::vec3(0, 1, 0)) *
		glm::rotate(glm::mat4(1.0f), glm::radians(-rotation.z), glm::vec3(0, 0, 1)) *
		glm::translate(glm::mat4(1.0f), -position));

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
		{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}} 
	};

	indices = {
		0, 1, 2, 2, 3, 0
	};
}

//this full screen quad is defined in NDC, it does multiply view matrix, so the winding is the opposite
void Mesh::InitFullScreenQuad()
{
	vertices = {
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
		{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
		{{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}
	};

	indices = {
		0, 1, 2, 2, 3, 0
	};
}


//adjacent triangles do not share vertices(vertices are unique)
void Mesh::InitFromFile(const std::string& fileName)
{
	LoadObjMesh(fileName);
}

void Mesh::LoadObjMesh(const std::string& fileName)
{
	std::fstream file;
	file.open(fileName, std::ios::in);
	if (file.is_open())
	{
		std::vector<glm::vec3> vecPos;
		std::vector<glm::vec3> vecNor;
		std::vector<glm::vec2> vecUV;
		std::vector<Point> vecPoint;

		std::string str;
		while (std::getline(file, str))
		{
			if (str.substr(0, 2) == "v ")
			{
				std::stringstream ss;
				ss << str.substr(2);
				glm::vec3 v;
				ss >> v.x;
				ss >> v.y;
				ss >> v.z;
				vecPos.push_back(v);
			}
			else if (str.substr(0, 3) == "vt ")
			{
				std::stringstream ss;
				ss << str.substr(3);
				glm::vec2 v;
				ss >> v.x;
				ss >> v.y;
				vecUV.push_back(v);
			}
			else if (str.substr(0, 3) == "vn ")
			{
				std::stringstream ss;
				ss << str.substr(3);
				glm::vec3 v;
				ss >> v.x;
				ss >> v.y;
				ss >> v.z;
				vecNor.push_back(v);
			}
			else if (str.substr(0, 2) == "f ")
			{
				std::stringstream ss;
				std::vector<Point> tempVecPoint;
				ss << str.substr(2);

				//Parsing
				ParseObjFace(ss, tempVecPoint);

				//if there are more than 3 vertices for one face then split it in to several triangles
				for (int i = 0; i < tempVecPoint.size(); i++)
				{
					if (i >= 3)
					{
						vecPoint.push_back(tempVecPoint.at(0));
						vecPoint.push_back(tempVecPoint.at(i - 1));
					}
					vecPoint.push_back(tempVecPoint.at(i));
				}

			}
			else if (str[0] == '#')
			{
				//comment
			}
			else
			{
				//others
			}
		}

		AssembleObjMesh(vecPos, vecUV, vecNor, vecPoint);

	}
	else
	{
		throw std::runtime_error("can not open: " + fileName);
	}

	file.close();
}

void Mesh::ParseObjFace(std::stringstream &ss, std::vector<Point>& tempVecPoint)
{
	char discard;
	char peek;
	uint32_t data;
	Point tempPoint;
	bool next;

	//One vertex in one loop
	do
	{
		next = false;
		tempPoint = { 0, 0, 0 };

		if (!next)
		{
			ss >> peek;
			if (!ss.eof() && peek >= '0' && peek <= '9')
			{
				ss.putback(peek);
				ss >> data;
				tempPoint.VI = data;//index start at 1 in an .obj file but at 0 in an array
				ss >> discard;
				if (!ss.eof())
				{
					if (discard == '/')
					{
						//do nothing, this is the normal case, we move on to the next property
					}
					else
					{
						//this happens when the current property is the last property of this point on the face
						//the discard is actually the starting number of the next point, so we put it back and move on
						ss.putback(discard);
						next = true;
					}
				}
				else
					next = true;
			}
			else
				next = true;
		}

		if (!next)
		{
			ss >> peek;
			if (!ss.eof() && peek >= '0' && peek <= '9')
			{
				ss.putback(peek);
				ss >> data;
				tempPoint.TI = data;//index start at 1 in an .obj file but at 0 in an array
				ss >> discard;
				if (!ss.eof())
				{
					if (discard == '/')
					{
						//do nothing, this is the normal case, we move on to the next property
					}
					else
					{
						//this happens when the current property is the last property of this point on the face
						//the discard is actually the starting number of the next point, so we put it back and move on
						ss.putback(discard);
						next = true;
					}
				}
				else
					next = true;
			}
			else
				next = true;
		}

		if (!next)
		{
			ss >> peek;
			if (!ss.eof() && peek >= '0' && peek <= '9')
			{
				ss.putback(peek);
				ss >> data;
				tempPoint.NI = data;//index start at 1 in an .obj file but at 0 in an array
				//normally we don't need the code below because normal index usually is the last property, but hey, better safe than sorry
				ss >> discard;
				if (!ss.eof())
				{
					if (discard == '/')
					{
						//do nothing, this is the normal case, we move on to the next property
					}
					else
					{
						//this happens when the current property is the last property of this point on the face
						//the discard is actually the starting number of the next point, so we put it back and move on
						ss.putback(discard);
						next = true;
					}
				}
				else
					next = true;
			}
			else
				next = true;
		}

		tempVecPoint.push_back(tempPoint);
	} while (!ss.eof());
}

void Mesh::AssembleObjMesh(
	const std::vector<glm::vec3> &vecPos,
	const std::vector<glm::vec2> &vecUV,
	const std::vector<glm::vec3> &vecNor,
	const std::vector<Point> &vecPoint)
{
	uint32_t n = static_cast<uint32_t>(vecPoint.size());

	vertices.resize(n);
	indices.resize(n);

	for (uint32_t i = 0; i < n; i++)
	{
		glm::vec3 pos(0, 0, 0);
		glm::vec3 nor(0, 0, 0);
		glm::vec2 uv(0, 0);

		if (vecPoint[i].VI > 0) pos = vecPos[vecPoint[i].VI - 1];//index start at 1 in an .obj file but at 0 in an array, 0 was used to mark not-have-pos
		if (vecPoint[i].NI > 0) nor = vecNor[vecPoint[i].NI - 1];//index start at 1 in an .obj file but at 0 in an array, 0 was used to mark not-have-nor
		if (vecPoint[i].TI > 0) uv = vecUV[vecPoint[i].TI - 1];//index start at 1 in an .obj file but at 0 in an array, 0 was used to mark not-have-uv

		vertices[i] = { pos, nor, uv };
		indices[i] = i;//this way, all 3 vertices on every triangle are unique, even though they belong to the same polygon, which increase storing space but allow for finer control
	}
}
