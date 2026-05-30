#pragma once

#include <glm/glm.hpp>

namespace tile
{
	bool RayTriangleIntersect(
		glm::vec3 orig, glm::vec3 dir,
		glm::vec3 v0, glm::vec3 v1, glm::vec3 v2,
		float& t, float& u, float& v);

	glm::vec3 ScreenToRay(
		const glm::mat4& viewProj,
		float mouseX, float mouseY,
		float winW, float winH);
}
