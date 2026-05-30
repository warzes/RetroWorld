#include "stdafx.h"
#include "TilePicking.h"
#include <glm/gtc/matrix_transform.hpp>

namespace tile
{
	bool RayTriangleIntersect(
		glm::vec3 orig, glm::vec3 dir,
		glm::vec3 v0, glm::vec3 v1, glm::vec3 v2,
		float& t, float& u, float& v)
	{
		constexpr float EPS = 1e-8f;
		glm::vec3 edge1 = v1 - v0;
		glm::vec3 edge2 = v2 - v0;
		glm::vec3 pvec  = glm::cross(dir, edge2);
		float det = glm::dot(edge1, pvec);
		if (std::abs(det) < EPS) return false;
		float invDet = 1.0f / det;
		glm::vec3 tvec = orig - v0;
		u = glm::dot(tvec, pvec) * invDet;
		if (u < 0.0f || u > 1.0f) return false;
		glm::vec3 qvec = glm::cross(tvec, edge1);
		v = glm::dot(dir, qvec) * invDet;
		if (v < 0.0f || u + v > 1.0f) return false;
		t = glm::dot(edge2, qvec) * invDet;
		return t > 0.0f;
	}

	glm::vec3 ScreenToRay(
		const glm::mat4& viewProj,
		float mouseX, float mouseY,
		float winW, float winH)
	{
		float ndcX = (2.0f * mouseX / winW - 1.0f);
		float ndcY = (1.0f - 2.0f * mouseY / winH);

		glm::mat4 invVP = glm::inverse(viewProj);
		glm::vec4 nearP = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
		glm::vec4 farP  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);

		nearP /= nearP.w;
		farP  /= farP.w;

		glm::vec3 dir = glm::vec3(farP - nearP);
		return glm::normalize(dir);
	}
}
