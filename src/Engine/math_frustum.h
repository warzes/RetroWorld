#pragma once

#include "math_plane.h"
#include "math_aabb.h"

namespace math
{
	struct Frustum final
	{
		std::array<Plane, 6> planes; // left, right, top, bottom, near, far
	};

	// Extract frustum from view-projection matrix (Gribb/Hartmann method)
	inline Frustum ExtractFrustum(const glm::mat4& viewProjection)
	{
		Frustum f;

		// Gribb/Hartmann: extract planes from rows of VP matrix (column-major storage)
		glm::vec4 row0 = glm::row(viewProjection, 0);
		glm::vec4 row1 = glm::row(viewProjection, 1);
		glm::vec4 row2 = glm::row(viewProjection, 2);
		glm::vec4 row3 = glm::row(viewProjection, 3);

		// Left:   row3 + row0
		f.planes[0].equation = row3 + row0;
		// Right:  row3 - row0
		f.planes[1].equation = row3 - row0;
		// Bottom: row3 + row1
		f.planes[2].equation = row3 + row1;
		// Top:    row3 - row1
		f.planes[3].equation = row3 - row1;
		// Near:   row3 + row2
		f.planes[4].equation = row3 + row2;
		// Far:    row3 - row2
		f.planes[5].equation = row3 - row2;

		// Normalize all planes
		for (auto& p : f.planes)
		{
			float len = glm::length(glm::vec3(p.equation));
			if (len > 0.0f)
				p.equation /= len;
		}

		return f;
	}

	// Test AABB against frustum using p-vertex / n-vertex optimization
	inline bool TestAABB(const Frustum& f, const AABB& box, const glm::mat4& worldTransform)
	{
		// Transform AABB to world space
		AABB worldBox = box.Transform(worldTransform);

		// Test each plane with p-vertex / n-vertex optimization
		for (const auto& plane : f.planes)
		{
			glm::vec3 n(plane.equation);

			// n-vertex: corner with minimum dot product with plane normal
			glm::vec3 nVertex = worldBox.min;
			if (n.x >= 0.0f) nVertex.x = worldBox.max.x;
			if (n.y >= 0.0f) nVertex.y = worldBox.max.y;
			if (n.z >= 0.0f) nVertex.z = worldBox.max.z;

			// If n-vertex is outside this plane, the whole AABB is outside
			if (glm::dot(n, nVertex) + plane.equation.w < 0.0f)
				return false;
		}

		return true;
	}

	// Test sphere against frustum
	inline bool TestSphere(const Frustum& f, const glm::vec3& center, float radius)
	{
		for (const auto& plane : f.planes)
		{
			glm::vec3 n(plane.equation);
			float dist = glm::dot(n, center) + plane.equation.w;
			if (dist < -radius)
				return false;
		}
		return true;
	}

} // namespace math