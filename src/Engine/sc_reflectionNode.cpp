#include "stdafx.h"
#include "sc_reflectionNode.h"
//=============================================================================
scene::ReflectionProbeNode::ReflectionProbeNode(std::string name)
	: SceneNode(std::move(name), NodeType::ReflectionProbe)
{}
//=============================================================================
std::array<glm::mat4, 6> scene::ReflectionProbeNode::GetCaptureMatrices() const
{
	glm::vec3 pos = glm::vec3(cachedWorldMatrix[3]);
	glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, radius);

	// Cubemap face directions: +X, -X, +Y, -Y, +Z, -Z
	// Standard OpenGL cubemap layout
	static const std::array<glm::vec3, 6> targets = {
		glm::vec3(1,  0,  0),
		glm::vec3(-1,  0,  0),
		glm::vec3(0,  1,  0),
		glm::vec3(0, -1,  0),
		glm::vec3(0,  0,  1),
		glm::vec3(0,  0, -1),
	};
	static const std::array<glm::vec3, 6> ups = {
		glm::vec3(0, -1,  0),
		glm::vec3(0, -1,  0),
		glm::vec3(0,  0,  1),
		glm::vec3(0,  0, -1),
		glm::vec3(0, -1,  0),
		glm::vec3(0, -1,  0),
	};

	std::array<glm::mat4, 6> result;
	for (int i = 0; i < 6; ++i)
		result[i] = proj * glm::lookAt(pos, pos + targets[i], ups[i]);
	return result;
}
//=============================================================================