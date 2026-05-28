#pragma once

namespace math
{
	struct Plane final
	{
		glm::vec4 equation; // ax + by + cz + d = 0 (normalized)
	};
} // namespace math