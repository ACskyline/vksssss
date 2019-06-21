#include "Camera.h"

Camera::Camera(const std::string& _name, const glm::vec3& _position, const glm::vec3& _target, const glm::vec3& _up, float _fov, uint32_t _width, uint32_t _height, float _near, float _far)
	: name(_name), position(_position), target(_target), up(_up), fov(_fov), width(_width), height(_height), near(_near), far(_far)
{
}

Camera::~Camera()
{
	CleanUp();
}

void Camera::InitCamera()
{
	//do nothing
}

void Camera::CleanUp()
{
	//do nothing
}

glm::mat4 Camera::GetProjectionMatrix()
{
	glm::mat4 mat = glm::perspective(glm::radians(fov), width / (float)height, near, far);
	mat[1][1] *= -1;
	return mat;
}

glm::mat4 Camera::GetViewMatrix()
{
	glm::mat4 mat = glm::lookAt(position, target, up);
	return mat;
}
