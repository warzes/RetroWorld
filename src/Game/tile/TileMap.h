#pragma once

#include "TileTypes.h"

namespace tile
{
	class TileMap final
	{
	public:
		TileMap() = default;
		TileMap(int w, int h);

		void Resize(int w, int h);
		bool InBounds(int x, int y) const noexcept;

		Tile&       Get(int x, int y);
		const Tile& Get(int x, int y) const;

		int GetWidth()  const noexcept { return width;  }
		int GetHeight() const noexcept { return height; }

		void GenerateRandom(uint32_t seed = 0);
		void Clear();
		void SetAll(TileSpaceType type);

		float GetFloorHeightAt(int tx, int ty, int corner) const noexcept;
		float GetCeilHeightAt(int tx, int ty, int corner) const noexcept;

		CornerInfo FindNearestCorner(const glm::vec3& worldPos, float threshold = 0.3f) const;

	private:
		int width  = 16;
		int height = 16;
		std::vector<Tile> tiles;

		void carveRoom(int x, int y, int w, int h);
		void carveCorridor(int x1, int y1, int x2, int y2);
	};
}
