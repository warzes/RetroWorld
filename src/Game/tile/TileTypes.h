#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string_view>
#include <glm/glm.hpp>

namespace tile
{
	enum class TileSpaceType : uint8_t
	{
		EMPTY = 0,
		SOLID,
	};

	enum class FaceDir : uint8_t
	{
		FLOOR = 0,
		CEILING,
		NORTH,
		SOUTH,
		EAST,
		WEST,
		COUNT
	};

	inline constexpr const char* FaceNames[] =
	{ "Floor", "Ceiling", "North", "South", "East", "West" };

	struct Tile final
	{
		TileSpaceType spaceType = TileSpaceType::EMPTY;
		bool renderSolid = false;
		uint8_t wallTex   = 0;
		uint8_t floorTex  = 1;
		uint8_t ceilTex   = 2;

		float floorHeight = -0.5f;
		float ceilHeight  =  0.5f;

		float slopeNW = 0.0f, slopeNE = 0.0f, slopeSE = 0.0f, slopeSW = 0.0f;
		float ceilSlopeNW = 0.0f, ceilSlopeNE = 0.0f, ceilSlopeSE = 0.0f, ceilSlopeSW = 0.0f;
	};

	struct HitInfo final
	{
		int tileX = -1, tileY = -1;
		FaceDir face = FaceDir::COUNT;
		int corner = -1;
		float t = 0.0f;
		glm::vec3 point{};
	};

	struct CornerInfo final
	{
		int tx{}, ty{};
		int corner{};
		glm::vec3 worldPos{};
		float dist{};
	};

	inline glm::vec3 CornerLocalPos(int corner, float height, float slope)
	{
		float x = (corner == 1 || corner == 2) ?  0.5f : -0.5f;
		float z = (corner == 2 || corner == 3) ?  0.5f : -0.5f;
		return { x, height + slope, z };
	}
}
