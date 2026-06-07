#pragma once

#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>

namespace ed
{
	constexpr float TILE_SPACING_DEFAULT = 2.0f;

	enum class Direction : uint8_t { Z_POS, Z_NEG, X_POS, X_NEG, Y_POS, Y_NEG };

	template<typename T>
	inline T Min(T a, T b) { return a < b ? a : b; }

	template<typename T>
	inline T Max(T a, T b) { return a < b ? b : a; }

	template<typename T>
	inline int Sign(T a) { if (a == 0) return 0; return a < 0 ? -1 : 1; }

	inline float ToRadians(float degrees) { return degrees * glm::pi<float>() / 180.0f; }
	inline float ToDegrees(float radians) { return (radians / glm::pi<float>()) * 180.0f; }

	inline int OffsetDegrees(int base, int add)
	{
		return (base + add >= 0) ? (base + add) % 360 : (360 + (base + add));
	}

	inline glm::mat4 TileRotationMatrix(uint8_t tileYaw, uint8_t tilePitch)
	{
		return glm::rotate(glm::mat4(1.0f), float(tilePitch % 4) * -glm::half_pi<float>(), glm::vec3(1, 0, 0))
			* glm::rotate(glm::mat4(1.0f), float(tileYaw % 4) * -glm::half_pi<float>(), glm::vec3(0, 1, 0));
	}

	// Get world-space NDC for frustum test
	inline glm::vec3 GetWorldToNDC(glm::vec3 position, const glm::mat4& view, const glm::mat4& proj)
	{
		glm::vec4 clipPos = proj * view * glm::vec4(position, 1.0f);
		if (clipPos.w == 0.0f) return glm::vec3(0.0f);
		return glm::vec3(clipPos) / clipPos.w;
	}
} // namespace ed