#pragma once
#include "GlobalInclude.h"

class Camera
{
public:
	Camera(
		const std::string& _name, 
		const glm::vec3& _position, 
		const glm::vec3& _target, 
		const glm::vec3& _up, 
		float _fov, 
		uint32_t _width, 
		uint32_t _height, 
		float _near,
		float _far);
	virtual ~Camera();

	void InitCamera();
	glm::mat4 GetProjectionMatrix();
	glm::mat4 GetViewMatrix();

	void CleanUp();

	glm::vec3 position;
	glm::vec3 target;
	glm::vec3 up;

private:
	std::string name;
	float fov;
	uint32_t width;
	uint32_t height;
	float near;
	float far;
};

class OrbitCamera :
	public Camera
{
public:
	OrbitCamera(
		const std::string& _name,
		float _distance,
		float _horizontalAngle,
		float _verticalAngle,
		const glm::vec3& _target,
		const glm::vec3& _up,
		float _fov,
		uint32_t _width,
		uint32_t _height,
		float _near,
		float _far);
	virtual ~OrbitCamera();

	void UpdatePosition();

	float distance;
	float horizontalAngle;
	float verticalAngle;
};
