#include "stdafx.h"
#include "sc_cameraNode.h"
//=============================================================================
scene::CameraNode::CameraNode(std::string name_)
	: SceneNode(std::move(name_), NodeType::Camera)
{}
//=============================================================================
glm::mat4 scene::CameraNode::GetViewMatrix() const
{
	if (externalCamera) return externalCamera->GetViewMatrix();
	glm::vec3 pos = glm::vec3(cachedWorldMatrix[3]);
	glm::vec3 forward = -glm::vec3(cachedWorldMatrix[2]);
	glm::vec3 up = glm::vec3(cachedWorldMatrix[1]);
	return glm::lookAt(pos, pos + forward, up);
}
//=============================================================================
glm::mat4 scene::CameraNode::GetProjectionMatrix() const
{
	return glm::perspective(fov, aspectRatio, nearPlane, farPlane);
}
//=============================================================================
glm::mat4 scene::CameraNode::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}
//=============================================================================
glm::vec3 scene::CameraNode::GetPosition() const
{
	if (externalCamera) return externalCamera->GetPosition();
	return glm::vec3(cachedWorldMatrix[3]);
}
//=============================================================================
math::Frustum scene::CameraNode::ExtractFrustum() const
{
	return math::ExtractFrustum(GetViewProjectionMatrix());
}
//=============================================================================