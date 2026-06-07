#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <functional>
#include <glm/glm.hpp>

#include "grid.hpp"
#include "editor_math.hpp"

namespace ed
{
	class MapMan;

	typedef int16_t TexID;
	typedef int16_t ModelID;

	constexpr TexID NO_TEX = -1;
	constexpr ModelID NO_MODEL = -1;
	constexpr int TEXTURES_PER_TILE = 2;

	struct Tile
	{
		ModelID shape;
		std::array<TexID, TEXTURES_PER_TILE> textures;
		uint8_t yaw, pitch;

		Tile()
			: shape(NO_MODEL), yaw(0), pitch(0)
		{
			textures[0] = NO_TEX;
			textures[1] = NO_TEX;
		}

		Tile(ModelID shape_, TexID tex1, TexID tex2, uint8_t yaw_, uint8_t pitch_)
			: shape(shape_), yaw(yaw_), pitch(pitch_)
		{
			textures[0] = tex1;
			textures[1] = tex2;
		}

		explicit operator bool() const { return shape != NO_MODEL; }
	};

	inline bool operator==(const Tile& lhs, const Tile& rhs)
	{
		if (lhs.shape != rhs.shape) return false;
		for (int i = 0; i < TEXTURES_PER_TILE; ++i)
			if (lhs.textures[i] != rhs.textures[i]) return false;
		return (lhs.yaw == rhs.yaw) && (lhs.pitch == rhs.pitch);
	}

	inline bool operator!=(const Tile& lhs, const Tile& rhs) { return !(lhs == rhs); }

	class TileGrid : public Grid<Tile>
	{
	public:
		TileGrid(MapMan& mapMan, size_t width, size_t height, size_t length);
		TileGrid(MapMan& mapMan, size_t width, size_t height, size_t length, float spacing, Tile fill);

		TileGrid(const TileGrid& other) = default;
		TileGrid& operator=(const TileGrid& other) = default;

		Tile GetTile(int i, int j, int k) const;
		Tile GetTile(int flatIndex) const;
		void SetTile(int i, int j, int k, const Tile& tile);
		void SetTile(int flatIndex, const Tile& tile);
		void SetTileRect(int i, int j, int k, int w, int h, int l, const Tile& tile);
		void CopyTiles(int i, int j, int k, const TileGrid& src, bool ignoreEmpty = false);
		void UnsetTile(int i, int j, int k);
		TileGrid Subsection(int i, int j, int k, int w, int h, int l) const;

		// Serialization
		std::string GetTileDataBase64() const;
		void SetTileDataBase64(std::string data);
		void SetTileDataBase64OldFormat(std::string data);

		std::pair<std::vector<TexID>, std::vector<ModelID>> GetUsedIDs() const;

	protected:
		std::reference_wrapper<MapMan> _mapMan;
	};
} // namespace ed