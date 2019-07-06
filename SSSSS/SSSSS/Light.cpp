#include "Light.h"

#include "Camera.h"

Light::Light(
	const std::string& _name,
	const glm::vec3& _color,
	const glm::vec3& _position,
	Camera* _pCamera,
	RenderTexture* _pRenderTexture) 
	:
	name(_name), 
	color(_color), 
	position(_position),
	pScene(nullptr),
	pCamera(_pCamera),
	pRenderTexture(_pRenderTexture),
	textureIndex(-1)
{
}

Light::~Light()
{
	CleanUp();
}

void Light::InitLight()
{
	//do nothing
}

void Light::CleanUp()
{
	//do nothing
}


const glm::vec3& Light::GetColor() const
{
	return color;
}

const glm::vec3& Light::GetPosition() const
{
	return position;
}

glm::mat4 Light::GetProjectionMatrix() const
{
	glm::mat4 mat(1);
	if (pCamera != nullptr)
	{
		return pCamera->GetProjectionMatrix();
	}
	return mat;
}

glm::mat4 Light::GetViewMatrix() const
{
	glm::mat4 mat(1);
	if (pCamera != nullptr)
	{
		return pCamera->GetViewMatrix();
	}
	return mat;
}

RenderTexture* Light::GetRenderTexturePtr() const
{
	return pRenderTexture;
}

int32_t Light::GetTextureIndex() const
{
	return textureIndex;
}

void Light::SetTextureIndex(int32_t index)
{
	textureIndex = index;
}

void Light::SetScene(Scene* _pScene)
{
	pScene = _pScene;
}