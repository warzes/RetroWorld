#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string_view>
#include <glm/glm.hpp>

namespace tile
{
    inline constexpr uint8_t TEX_NOT_SET = 0xFF;

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

		// General textures (local index 0-63 per atlas)
		uint8_t wallTex       = 0;  // Upper wall (also fallback for lower)
		uint8_t wallBottomTex = 0;  // Lower wall
		uint8_t floorTex      = 1;
		uint8_t ceilTex       = 2;

		// Atlas IDs (0=T1, 1=T2)
		uint8_t wallAtlas       = 0;
		uint8_t wallBottomAtlas = 0;
		uint8_t floorAtlas      = 0;
		uint8_t ceilAtlas       = 0;

		// Per-direction upper wall overrides (TEX_NOT_SET = use wallTex/wallAtlas)
		uint8_t northTex = TEX_NOT_SET;
		uint8_t southTex = TEX_NOT_SET;
		uint8_t eastTex  = TEX_NOT_SET;
		uint8_t westTex  = TEX_NOT_SET;

		uint8_t northAtlas = 0;
		uint8_t southAtlas = 0;
		uint8_t eastAtlas  = 0;
		uint8_t westAtlas  = 0;

		// Per-direction lower wall overrides (TEX_NOT_SET = use wallBottomTex → wallTex)
		uint8_t bottomNorthTex = TEX_NOT_SET;
		uint8_t bottomSouthTex = TEX_NOT_SET;
		uint8_t bottomEastTex  = TEX_NOT_SET;
		uint8_t bottomWestTex  = TEX_NOT_SET;

		uint8_t bottomNorthAtlas = 0;
		uint8_t bottomSouthAtlas = 0;
		uint8_t bottomEastAtlas  = 0;
		uint8_t bottomWestAtlas  = 0;

        float floorHeight = -0.5f;
        float ceilHeight  =  0.5f;

        float slopeNW = 0.0f, slopeNE = 0.0f, slopeSE = 0.0f, slopeSW = 0.0f;
        float ceilSlopeNW = 0.0f, ceilSlopeNE = 0.0f, ceilSlopeSE = 0.0f, ceilSlopeSW = 0.0f;

        [[nodiscard]] uint8_t GetWallTex(FaceDir dir) const noexcept
        {
            switch (dir)
            {
            case FaceDir::NORTH: return northTex != TEX_NOT_SET ? northTex : wallTex;
            case FaceDir::SOUTH: return southTex != TEX_NOT_SET ? southTex : wallTex;
            case FaceDir::EAST:  return eastTex  != TEX_NOT_SET ? eastTex  : wallTex;
            case FaceDir::WEST:  return westTex  != TEX_NOT_SET ? westTex  : wallTex;
            default: return wallTex;
            }
        }

		[[nodiscard]] uint8_t GetWallBottomTex(FaceDir dir) const noexcept
		{
			// Per-direction lower override
			uint8_t pd = wallTex;
			switch (dir)
			{
			case FaceDir::NORTH: pd = bottomNorthTex; break;
			case FaceDir::SOUTH: pd = bottomSouthTex; break;
			case FaceDir::EAST:  pd = bottomEastTex;  break;
			case FaceDir::WEST:  pd = bottomWestTex;  break;
			default: break;
			}
			if (pd != TEX_NOT_SET) return pd;
			// Fallback: general lower wall texture
			if (wallBottomTex != TEX_NOT_SET) return wallBottomTex;
			// Ultimate fallback: upper wall texture
			return GetWallTex(dir);
		}

		[[nodiscard]] uint8_t GetWallAtlas(FaceDir dir) const noexcept
		{
			switch (dir)
			{
			case FaceDir::NORTH: return northTex != TEX_NOT_SET ? northAtlas : wallAtlas;
			case FaceDir::SOUTH: return southTex != TEX_NOT_SET ? southAtlas : wallAtlas;
			case FaceDir::EAST:  return eastTex  != TEX_NOT_SET ? eastAtlas  : wallAtlas;
			case FaceDir::WEST:  return westTex  != TEX_NOT_SET ? westAtlas  : wallAtlas;
			default: return wallAtlas;
			}
		}

		[[nodiscard]] uint8_t GetWallBottomAtlas(FaceDir dir) const noexcept
		{
			uint8_t pd = wallAtlas;
			switch (dir)
			{
			case FaceDir::NORTH: pd = bottomNorthTex != TEX_NOT_SET ? bottomNorthAtlas : wallBottomAtlas; break;
			case FaceDir::SOUTH: pd = bottomSouthTex != TEX_NOT_SET ? bottomSouthAtlas : wallBottomAtlas; break;
			case FaceDir::EAST:  pd = bottomEastTex  != TEX_NOT_SET ? bottomEastAtlas  : wallBottomAtlas; break;
			case FaceDir::WEST:  pd = bottomWestTex  != TEX_NOT_SET ? bottomWestAtlas  : wallBottomAtlas; break;
			default: break;
			}
			return pd;
		}

		[[nodiscard]] int GlobalTexIndex(uint8_t tex, uint8_t atlas) const noexcept
		{
			return static_cast<int>(atlas) * 64 + static_cast<int>(tex);
		}
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
