#include "Light.h"

#include "Camera.h"

Light::Light(
	const std::string& _name,
	const glm::vec3& _color,
	const glm::vec3& _position,
	Camera* _pCamera,
	RenderTexture* _pRenderTexture,
	RenderTexture* _pRenderTexture2) 
	:
	name(_name), 
	color(_color), 
	position(_position),
	pScene(nullptr),
	pCamera(_pCamera),
	pRenderTexture(_pRenderTexture),
	pRenderTexture2(_pRenderTexture2),
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

float Light::GetNear() const
{
	if (pCamera != nullptr)
		return pCamera->GetNear();
	else
		return 0.0f;
}

float Light::GetFar() const
{
	if (pCamera != nullptr)
		return pCamera->GetFar();
	else
		return 0.0f;
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
	if (pCamera != nullptr)
		return pCamera->GetProjectionMatrix();
	else
		return glm::mat4(1);
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

RenderTexture* Light::GetRenderTexturePtr2() const
{
	return pRenderTexture2 == nullptr ? pRenderTexture : pRenderTexture2;
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