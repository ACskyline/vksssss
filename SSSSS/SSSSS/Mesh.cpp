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

VkDescriptorSet* Mesh::GetObjectDescriptorSetPtr(int frame)
{
	return objectDescriptorSetVec.data() + frame;
}

int Mesh::GetObjectDescriptorSetCount() const
{
	return objectDescriptorSetVec.size();
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
	CreateObjectUniformBuffer(_pRenderer->frameCount);
	pRenderer->CreateDescriptorSetLayout(
		objectDescriptorSetLayout,
		0,
		oUboCount,//only 1 oUBO
		oUboCount,//only 1 oUBO, so offset is 1
		static_cast<uint32_t>(pTextureVec.size()));
	objectDescriptorSetVec.resize(_pRenderer->frameCount);
	pRenderer->CreateDescriptorSets(
		objectDescriptorSetVec,
		descriptorPool,
		objectDescriptorSetLayout);

	VkDeviceSize uboSize = sizeof(oUBO);
	for (int i = 0; i < objectDescriptorSetVec.size(); i++)
	{
		//bind ubo
		VkDeviceSize uboOffset = pRenderer->GetAlignedUboOffset(uboSize, i);
		pRenderer->BindUniformBufferToDescriptorSets(objectUniformBuffer, uboOffset, uboSize, { objectDescriptorSetVec[i] }, 0);//only 1 oUBO

		//bind texture
		for (int j = 0; j < pTextureVec.size(); j++)
		{
			pRenderer->BindTextureToDescriptorSets(pTextureVec[j]->GetTextureImageView(), pTextureVec[j]->GetSampler(), { objectDescriptorSetVec[i] }, oUboCount + j);//only 1 oUBO, so binding offset is 1
		}
	}
}

void Mesh::UpdateObjectUniformBuffer(int frame)
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
	VkDeviceSize size = sizeof(oUBO);
	VkDeviceSize offset = pRenderer->GetAlignedUboOffset(size, frame);
	vkMapMemory(pRenderer->GetDevice(), objectUniformBufferMemory, offset, size, 0, &data);
	memcpy(data, &oUBO, sizeof(oUBO));
	vkUnmapMemory(pRenderer->GetDevice(), objectUniformBufferMemory);
}

void Mesh::CreateObjectUniformBuffer(int frameCount)
{
	VkDeviceSize bufferSize = pRenderer->GetAlignedUboSize(sizeof(ObjectUniformBufferObject)) * frameCount;

	pRenderer->CreateBuffer(bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		objectUniformBuffer,
		objectUniformBufferMemory);

	//initial update
	for (int i = 0; i < frameCount; i++)
	{
		UpdateObjectUniformBuffer(i);
	}
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
		{{-0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
		{{-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f},  {1.0f, 0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}
	};

	indices = {
		0, 1, 2, 2, 3, 0
	};
}

//this full screen quad is defined in NDC, it does multiply view matrix, so the winding is the opposite
void Mesh::InitFullScreenQuad()
{
	vertices = {
		{{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
		{{-1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
		{{1.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
		{{1.0f, -1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}}
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

	for (uint32_t i = 0; i < n; i += 3)
	{
		glm::vec3 p0 = vecPos[vecPoint[i].VI - 1];
		glm::vec3 p1 = vecPos[vecPoint[i + 1].VI - 1];
		glm::vec3 p2 = vecPos[vecPoint[i + 2].VI - 1];

		glm::vec2 w0 = vecUV[vecPoint[i].TI - 1];
		glm::vec2 w1 = vecUV[vecPoint[i + 1].TI - 1];
		glm::vec2 w2 = vecUV[vecPoint[i + 2].TI - 1];

		float x1 = p1.x - p0.x;
		float x2 = p2.x - p0.x;
		float y1 = p1.y - p0.y;
		float y2 = p2.y - p0.y;
		float z1 = p1.z - p0.z;
		float z2 = p2.z - p0.z;

		float s1 = w1.s - w0.s;
		float s2 = w2.s - w0.s;
		float t1 = w1.t - w0.t;
		float t2 = w2.t - w0.t;

		float r = 1.0f / (s1 * t2 - s2 * t1);
		glm::vec3 sdir(
			(t2 * x1 - t1 * x2) * r, 
			(t2 * y1 - t1 * y2) * r,
			(t2 * z1 - t1 * z2) * r);
		glm::vec3 tdir(
			(s1 * x2 - s2 * x1) * r, 
			(s1 * y2 - s2 * y1) * r,
			(s1 * z2 - s2 * z1) * r);

		sdir = glm::normalize(sdir);
		tdir = glm::normalize(tdir);

		for (uint32_t j = i; j < i + 3; j++)
		{
			glm::vec3 pos(0, 0, 0);
			glm::vec3 nor(0, 0, 0);
			glm::vec2 uv(0, 0);

			if (vecPoint[j].VI > 0) pos = vecPos[vecPoint[j].VI - 1];//index start at 1 in an .obj file but at 0 in an array, 0 was used to mark not-have-pos
			if (vecPoint[j].NI > 0) nor = vecNor[vecPoint[j].NI - 1];//index start at 1 in an .obj file but at 0 in an array, 0 was used to mark not-have-nor
			if (vecPoint[j].TI > 0) uv = vecUV[vecPoint[j].TI - 1];//index start at 1 in an .obj file but at 0 in an array, 0 was used to mark not-have-uv

			//The order is fixed, it is the same in the shaders. The only way to tell the direction when reconstructing tangent is the sign component.
			float sign = glm::dot(glm::cross(nor, sdir), tdir) > 0.0f ? 1.0f : -1.0f;
			glm::vec4 tan(sdir, sign);

			vertices[j] = { pos, nor, tan, uv };
			indices[j] = j;//this way, all 3 vertices on every triangle are unique, even though they belong to the same polygon, which increase storing space but allow for finer control
		}
	}
}