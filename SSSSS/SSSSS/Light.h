#pragma once
#include "GlobalInclude.h"

class Scene;
class Camera;
class RenderTexture;

class Light
{
public:
	Light(
		const std::string& _name,
		const glm::vec3& _color,
		const glm::vec3& _position,
		Camera* _pCamera,
		RenderTexture* _pRenderTexture,
		RenderTexture* _pRenderTexture2 = nullptr);
	~Light();

	float GetNear() const;
	float GetFar() const;
	const glm::vec3& GetColor() const;
	const glm::vec3& GetPosition() const;
	glm::mat4 GetProjectionMatrix() const;
	glm::mat4 GetViewMatrix() const;
	RenderTexture* GetRenderTexturePtr() const;
	RenderTexture* GetRenderTexturePtr2() const;//for TSM
	int32_t GetTextureIndex() const;

	void SetTextureIndex(int32_t index);
	void SetScene(Scene* _pScene);

	void InitLight();
	void CleanUp();

private:
	int32_t textureIndex;//corresponding texture index in a scene
	std::string name;
	glm::vec3 color;
	glm::vec3 position;
	Scene* pScene;//a light is bound to a scene
	Camera* pCamera;
	RenderTexture* pRenderTexture;
	RenderTexture* pRenderTexture2;//for TSM
};
