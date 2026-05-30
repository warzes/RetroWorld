#include "stdafx.h"
#include "TileMap.h"
#include <random>
#include <algorithm>
#include <glm/glm.hpp>

namespace tile
{
	TileMap::TileMap(int w, int h)
		: width(w), height(h), tiles(static_cast<size_t>(w) * static_cast<size_t>(h)) {}

	void TileMap::Resize(int w, int h)
	{
		width  = w;
		height = h;
		tiles.resize(static_cast<size_t>(w) * static_cast<size_t>(h));
	}

	bool TileMap::InBounds(int x, int y) const noexcept
	{
		return x >= 0 && x < width && y >= 0 && y < height;
	}

	Tile& TileMap::Get(int x, int y) { return tiles[static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(width)]; }

	const Tile& TileMap::Get(int x, int y) const { return tiles[static_cast<size_t>(x) + static_cast<size_t>(y) * static_cast<size_t>(width)]; }

	void TileMap::Clear()
	{
		for (auto& t : tiles) t = Tile{};
	}

	void TileMap::SetAll(TileSpaceType type)
	{
		for (auto& t : tiles)
		{
			t.spaceType   = type;
			t.renderSolid = (type == TileSpaceType::SOLID);
		}
	}

	float TileMap::GetFloorHeightAt(int tx, int ty, int corner) const noexcept
	{
		if (!InBounds(tx, ty)) return -0.5f;
		const auto& t = Get(tx, ty);
		switch (corner)
		{
			case 0: return t.floorHeight + t.slopeNW;
			case 1: return t.floorHeight + t.slopeNE;
			case 2: return t.floorHeight + t.slopeSE;
			case 3: return t.floorHeight + t.slopeSW;
			default: return t.floorHeight;
		}
	}

	float TileMap::GetCeilHeightAt(int tx, int ty, int corner) const noexcept
	{
		if (!InBounds(tx, ty)) return 0.5f;
		const auto& t = Get(tx, ty);
		switch (corner)
		{
			case 0: return t.ceilHeight + t.slopeNW;
			case 1: return t.ceilHeight + t.slopeNE;
			case 2: return t.ceilHeight + t.slopeSE;
			case 3: return t.ceilHeight + t.slopeSW;
			default: return t.ceilHeight;
		}
	}

	CornerInfo TileMap::FindNearestCorner(const glm::vec3& worldPos, float threshold) const
	{
		CornerInfo best;
		best.dist = threshold;

		for (int ty = 0; ty < height; ++ty)
		{
			for (int tx = 0; tx < width; ++tx)
			{
				if (Get(tx, ty).spaceType != TileSpaceType::SOLID) continue;
				for (int c = 0; c < 4; ++c)
				{
					float h = GetFloorHeightAt(tx, ty, c);
					float px = static_cast<float>(tx) + (c == 1 || c == 2 ? 0.5f : -0.5f);
					float pz = static_cast<float>(ty) + (c == 2 || c == 3 ? 0.5f : -0.5f);
					glm::vec3 pos{ px, h, pz };
					float d = glm::distance(worldPos, pos);
					if (d < best.dist)
					{
						best.dist = d;
						best.tx   = tx;
						best.ty   = ty;
						best.corner = c;
						best.worldPos = pos;
					}
				}
			}
		}
		return best;
	}

	void TileMap::GenerateRandom(uint32_t seed)
	{
		Clear();
		SetAll(TileSpaceType::SOLID);

		std::mt19937 rng(seed ? seed : static_cast<uint32_t>(std::random_device{}()));

		const int roomCount = 5 + static_cast<int>(rng() % 4);
		struct Room { int x, y, w, h; };
		std::vector<Room> rooms;
		rooms.reserve(static_cast<size_t>(roomCount));
		std::uniform_int_distribution<int> rxDist(1, width - 5);
		std::uniform_int_distribution<int> ryDist(1, height - 5);
		std::uniform_int_distribution<int> rwDist(3, 6);
		std::uniform_int_distribution<int> rhDist(3, 6);

		for (int i = 0; i < roomCount * 3; ++i)
		{
			if (rooms.size() >= static_cast<size_t>(roomCount)) break;
			int rx = rxDist(rng);
			int ry = ryDist(rng);
			int rw = rwDist(rng);
			int rh = rhDist(rng);

			if (rx + rw >= width - 1)  rw = width - 2 - rx;
			if (ry + rh >= height - 1) rh = height - 2 - ry;
			if (rw < 2 || rh < 2) continue;

			bool overlap = false;
			for (auto& r : rooms)
			{
				if (rx < r.x + r.w + 1 && rx + rw + 1 > r.x &&
					ry < r.y + r.h + 1 && ry + rh + 1 > r.y)
				{
					overlap = true;
					break;
				}
			}
			if (overlap) continue;

			carveRoom(rx, ry, rw, rh);
			rooms.push_back({ rx, ry, rw, rh });
		}

		if (rooms.size() >= 2)
		{
			for (size_t i = 1; i < rooms.size(); ++i)
			{
				auto& prev = rooms[i - 1];
				auto& curr = rooms[i];
				int x1 = prev.x + prev.w / 2;
				int y1 = prev.y + prev.h / 2;
				int x2 = curr.x + curr.w / 2;
				int y2 = curr.y + curr.h / 2;
				carveCorridor(x1, y1, x2, y2);
			}
		}
	}

	void TileMap::carveRoom(int x, int y, int w, int h)
	{
		std::mt19937 rng(static_cast<uint32_t>(std::random_device{}()));
		uint8_t floorTex = 1 + (rng() % 4);
		for (int dy = 0; dy < h; ++dy)
		{
			for (int dx = 0; dx < w; ++dx)
			{
				int tx = x + dx;
				int ty = y + dy;
				if (!InBounds(tx, ty)) continue;
				auto& t = Get(tx, ty);
				t.spaceType   = TileSpaceType::EMPTY;
				t.renderSolid = false;
				t.floorTex = floorTex;
			}
		}
	}

	void TileMap::carveCorridor(int x1, int y1, int x2, int y2)
	{
		int cx = x1, cy = y1;
		while (cx != x2)
		{
			if (InBounds(cx, cy))
			{
				auto& t = Get(cx, cy);
				t.spaceType   = TileSpaceType::EMPTY;
				t.renderSolid = false;
				t.floorTex    = 1;
			}
			cx += (cx < x2) ? 1 : -1;
		}
		while (cy != y2)
		{
			if (InBounds(cx, cy))
			{
				auto& t = Get(cx, cy);
				t.spaceType   = TileSpaceType::EMPTY;
				t.renderSolid = false;
				t.floorTex    = 1;
			}
			cy += (cy < y2) ? 1 : -1;
		}
	}
}
