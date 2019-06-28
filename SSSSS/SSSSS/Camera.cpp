#include "Camera.h"

Camera::Camera(
	const std::string& _name, 
	const glm::vec3& _position, 
	const glm::vec3& _target, 
	const glm::vec3& _up, 
	float _fov, 
	uint32_t _width, 
	uint32_t _height, 
	float _near, 
	float _far)
	: 
	name(_name), 
	position(_position), 
	target(_target), 
	up(_up), 
	fov(_fov), 
	width(_width), 
	height(_height), 
	near(_near), 
	far(_far)
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



OrbitCamera::OrbitCamera(
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
	float _far) 
	:
	Camera(
		_name, 
		glm::vec3(0,0,0), 
		_target, 
		_up, 
		_fov, 
		_width, 
		_height,
		_near, 
		_far),
	distance(_distance), 
	horizontalAngle(_horizontalAngle), 
	verticalAngle(_verticalAngle)

{
	UpdatePosition();
}


OrbitCamera::~OrbitCamera()
{
}

void OrbitCamera::UpdatePosition()
{
	float y = distance * glm::sin(glm::radians(verticalAngle));
	float x = distance * glm::cos(glm::radians(verticalAngle)) * glm::sin(glm::radians(horizontalAngle));
	float z = distance * glm::cos(glm::radians(verticalAngle)) * glm::cos(glm::radians(horizontalAngle));

	position = glm::vec3(target.x + x, target.y + y, target.z + z);
}