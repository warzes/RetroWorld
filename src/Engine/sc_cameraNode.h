#pragma once

#include "sc_node.h"
#include "gr_camera.h"
#include "math_frustum.h"

namespace scene
{
	class CameraNode final : public SceneNode
	{
	public:
		explicit CameraNode(std::string name);

		glm::mat4 GetViewMatrix() const;
		glm::mat4 GetProjectionMatrix() const;
		glm::mat4 GetViewProjectionMatrix() const;

		math::Frustum ExtractFrustum() const;

		gr::Camera camera{ glm::vec3(0.0f, 0.0f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f) };
		float fov = glm::radians(65.0f);
		float nearPlane = 0.1f;
		float farPlane = 1000.0f;
		float aspectRatio = 16.0f / 9.0f;
	};
} // namespace scene