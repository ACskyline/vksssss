#pragma once
#include "Renderer.h"
#include "GlobalInclude.h"

class Renderer;

class Camera
{
public:
	Camera(const std::string& _name, const glm::vec3& _position, const glm::vec3& _target, const glm::vec3& _up, float _fov, uint32_t _width, uint32_t _height, float _near, float _far);
	~Camera();

	void InitCamera(Renderer* pRenderer);
	glm::mat4 GetProjectionMatrix();
	glm::mat4 GetViewMatrix();

	void CleanUp();

private:
	std::string name;
	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 up;
	float fov;
	uint32_t width;
	uint32_t height;
	float near;
	float far;

};
