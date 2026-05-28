#pragma once

#include "math_plane.h"
#include "math_aabb.h"

namespace math
{
	struct Frustum final
	{
		std::array<Plane, 6> planes; // left, right, bottom, top, near, far
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

		// Test each plane with p-vertex / n-vertex optimisation.
		// Plane normals point inward. If even the "most inside"
		// corner (p-vertex, max dot product) is outside → whole AABB is outside.
		for (const auto& plane : f.planes)
		{
			const glm::vec3 n(plane.equation);          // inward-pointing normal

			// p-vertex: corner with the maximum dot(n, corner)
			glm::vec3 pVertex = worldBox.max;
			if (n.x < 0.0f) pVertex.x = worldBox.min.x;
			if (n.y < 0.0f) pVertex.y = worldBox.min.y;
			if (n.z < 0.0f) pVertex.z = worldBox.min.z;

			if (glm::dot(n, pVertex) + plane.equation.w < 0.0f)
				return false;                           // entirely outside this plane
		}

		return true;                                    // at least partially inside
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