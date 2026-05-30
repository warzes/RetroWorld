#include "stdafx.h"
#include "tile.h"
#include <random>
#include <algorithm>
#include <cfloat>
#include <glm/gtc/matrix_transform.hpp>
// ==========================================================================
namespace tile
{
	// ==========================================================================
	//  TileMap
	// ==========================================================================
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

	// --------------------------------------------------------------------------
	//  Random dungeon generation
	// --------------------------------------------------------------------------
	void TileMap::GenerateRandom(uint32_t seed)
	{
		Clear();
		SetAll(TileSpaceType::SOLID);

		std::mt19937 rng(seed ? seed : static_cast<uint32_t>(std::random_device{}()));

		// carve 5-8 rooms
		const int roomCount = 5 + static_cast<int>(rng() % 4);
		struct Room { int x, y, w, h; };
		std::vector<Room> rooms;
		rooms.reserve(static_cast<size_t>(roomCount));
		std::uniform_int_distribution<int> rxDist(1, width - 5);
		std::uniform_int_distribution<int> ryDist(1, height - 5);
		std::uniform_int_distribution<int> rwDist(3, 6);
		std::uniform_int_distribution<int> rhDist(3, 6);

		for (int i = 0; i < roomCount * 3; ++i) // try more to get enough
		{
			if (rooms.size() >= static_cast<size_t>(roomCount)) break;
			int rx = rxDist(rng);
			int ry = ryDist(rng);
			int rw = rwDist(rng);
			int rh = rhDist(rng);

			// clamp
			if (rx + rw >= width - 1)  rw = width - 2 - rx;
			if (ry + rh >= height - 1) rh = height - 2 - ry;
			if (rw < 2 || rh < 2) continue;

			// check overlap with existing rooms
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

		// connect rooms with corridors
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
		uint8_t floorTex = 1 + (rng() % 4); // vary floor texture
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

	// ==========================================================================
	//  TileMeshGen
	// ==========================================================================
	void TileMeshGen::Clear()
	{
		positions.clear();
		normals.clear();
		uvs.clear();
		indices.clear();
		triTileX.clear();
		triTileY.clear();
		triFace.clear();
	}

	void TileMeshGen::addQuad(
		glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
		glm::vec3 n,
		glm::vec2 uv00, glm::vec2 uv10, glm::vec2 uv11, glm::vec2 uv01,
		int tx, int ty, int faceIdx)
	{
		uint32_t base = static_cast<uint32_t>(positions.size());
		positions.push_back(a); normals.push_back(n); uvs.push_back(uv00);
		positions.push_back(b); normals.push_back(n); uvs.push_back(uv10);
		positions.push_back(c); normals.push_back(n); uvs.push_back(uv11);
		positions.push_back(d); normals.push_back(n); uvs.push_back(uv01);

		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
		indices.push_back(base + 0);

		for (int i = 0; i < 2; ++i)
		{
			triTileX.push_back(tx);
			triTileY.push_back(ty);
			triFace.push_back(faceIdx);
		}
	}

	void TileMeshGen::addTri(
		glm::vec3 a, glm::vec3 b, glm::vec3 c,
		glm::vec3 n,
		glm::vec2 uvA, glm::vec2 uvB, glm::vec2 uvC,
		int tx, int ty, int faceIdx)
	{
		uint32_t base = static_cast<uint32_t>(positions.size());
		positions.push_back(a); normals.push_back(n); uvs.push_back(uvA);
		positions.push_back(b); normals.push_back(n); uvs.push_back(uvB);
		positions.push_back(c); normals.push_back(n); uvs.push_back(uvC);

		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);

		triTileX.push_back(tx);
		triTileY.push_back(ty);
		triFace.push_back(faceIdx);
	}

	void TileMeshGen::BuildFromMap(const TileMap& map, int atlasDim,
		int highlightTX, int highlightTY, uint8_t highlightTex)
	{
		Clear();
		float invDim = 1.0f / static_cast<float>(atlasDim);

		auto isHighlighted = [&](int tx, int ty) noexcept -> bool {
			return tx == highlightTX && ty == highlightTY;
		};

		for (int ty = 0; ty < map.GetHeight(); ++ty)
		{
			for (int tx = 0; tx < map.GetWidth(); ++tx)
			{
				const Tile& tile = map.Get(tx, ty);
				if (tile.spaceType != TileSpaceType::SOLID) continue;

				// Floor — 2 triangles, CCW from above → normal +Y
				{
					int ti = isHighlighted(tx, ty) ? highlightTex : tile.floorTex;
					float u0 = static_cast<float>(ti % atlasDim) * invDim;
					float v0 = static_cast<float>(ti / atlasDim) * invDim;
					float u1 = u0 + invDim, v1 = v0 + invDim;

					float ftx = static_cast<float>(tx);
					float fty = static_cast<float>(ty);
					glm::vec3 nw{ ftx - 0.5f, map.GetFloorHeightAt(tx, ty, 0), fty - 0.5f };
					glm::vec3 ne{ ftx + 0.5f, map.GetFloorHeightAt(tx, ty, 1), fty - 0.5f };
					glm::vec3 se{ ftx + 0.5f, map.GetFloorHeightAt(tx, ty, 2), fty + 0.5f };
					glm::vec3 sw{ ftx - 0.5f, map.GetFloorHeightAt(tx, ty, 3), fty + 0.5f };

					// cross(se-nw, ne-nw) → +Y
					glm::vec3 n = glm::normalize(glm::cross(se - nw, ne - nw));

					// Triangles: NW→SE→NE, NW→SW→SE  (CCW from above)
					addTri(nw, se, ne, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u1, v1 }, glm::vec2{ u1, v0 },
						tx, ty, static_cast<int>(FaceDir::FLOOR));

					addTri(nw, sw, se, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u0, v1 }, glm::vec2{ u1, v1 },
						tx, ty, static_cast<int>(FaceDir::FLOOR));
				}

				// Ceiling — 2 triangles, CW from above → front face from below → normal -Y
				{
					int ti = isHighlighted(tx, ty) ? highlightTex : tile.ceilTex;
					float u0 = static_cast<float>(ti % atlasDim) * invDim;
					float v0 = static_cast<float>(ti / atlasDim) * invDim;
					float u1 = u0 + invDim, v1 = v0 + invDim;

					float ftx = static_cast<float>(tx);
					float fty = static_cast<float>(ty);
					glm::vec3 nw{ ftx - 0.5f, map.GetCeilHeightAt(tx, ty, 0), fty - 0.5f };
					glm::vec3 ne{ ftx + 0.5f, map.GetCeilHeightAt(tx, ty, 1), fty - 0.5f };
					glm::vec3 se{ ftx + 0.5f, map.GetCeilHeightAt(tx, ty, 2), fty + 0.5f };
					glm::vec3 sw{ ftx - 0.5f, map.GetCeilHeightAt(tx, ty, 3), fty + 0.5f };

					// cross(ne-nw, se-nw) → -Y
					glm::vec3 n = glm::normalize(glm::cross(ne - nw, se - nw));

					// Triangles: NW→NE→SE, NW→SE→SW  (CW from above, CCW from below)
					addTri(nw, ne, se, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u1, v0 }, glm::vec2{ u1, v1 },
						tx, ty, static_cast<int>(FaceDir::CEILING));

					addTri(nw, se, sw, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u1, v1 }, glm::vec2{ u0, v1 },
						tx, ty, static_cast<int>(FaceDir::CEILING));
				}

				// Walls  —  only if adjacent tile is EMPTY
				float ftx = static_cast<float>(tx);
				float fty = static_cast<float>(ty);
				auto addWallIf = [&](int ntx, int nty, FaceDir dir, uint8_t tex)
				{
					if (!map.InBounds(ntx, nty) ||
						map.Get(ntx, nty).spaceType != TileSpaceType::SOLID)
					{
						float ti = static_cast<float>(tex);
						float u0 = (std::fmod(ti, static_cast<float>(atlasDim))) * invDim;
						float v0 = (std::floor(ti * invDim)) * invDim;
						float u1 = u0 + invDim, v1 = v0 + invDim;

						// Wall vertices at the boundary
						float fNW = 0.0f, fNE = 0.0f, fSW = 0.0f, fSE = 0.0f;
						float cNW = 0.0f, cNE = 0.0f, cSW = 0.0f, cSE = 0.0f;

						switch (dir)
						{
						case FaceDir::NORTH: // z = ty - 0.5
							fNW = map.GetFloorHeightAt(tx, ty, 0);
							fNE = map.GetFloorHeightAt(tx, ty, 1);
							cNW = map.GetCeilHeightAt(tx, ty, 0);
							cNE = map.GetCeilHeightAt(tx, ty, 1);
							addQuad(
								{ ftx - 0.5f, fNW, fty - 0.5f }, // bl
								{ ftx + 0.5f, fNE, fty - 0.5f }, // br
								{ ftx + 0.5f, cNE, fty - 0.5f }, // tr
								{ ftx - 0.5f, cNW, fty - 0.5f }, // tl
								{ 0, 0, -1 },
								{ u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 },
								tx, ty, static_cast<int>(FaceDir::NORTH));
							break;

						case FaceDir::SOUTH: // z = ty + 0.5
							fSW = map.GetFloorHeightAt(tx, ty, 3);
							fSE = map.GetFloorHeightAt(tx, ty, 2);
							cSW = map.GetCeilHeightAt(tx, ty, 3);
							cSE = map.GetCeilHeightAt(tx, ty, 2);
							addQuad(
								{ ftx + 0.5f, fSE, fty + 0.5f }, // bl
								{ ftx - 0.5f, fSW, fty + 0.5f }, // br
								{ ftx - 0.5f, cSW, fty + 0.5f }, // tr
								{ ftx + 0.5f, cSE, fty + 0.5f }, // tl
								{ 0, 0, 1 },
								{ u0, v0 }, { u1, v0 }, { u1, v1 }, { u0, v1 },
								tx, ty, static_cast<int>(FaceDir::SOUTH));
							break;

						case FaceDir::EAST: // x = tx + 0.5
							fNE = map.GetFloorHeightAt(tx, ty, 1);
							fSE = map.GetFloorHeightAt(tx, ty, 2);
							cNE = map.GetCeilHeightAt(tx, ty, 1);
							cSE = map.GetCeilHeightAt(tx, ty, 2);
							addQuad(
								{ ftx + 0.5f, fSE, fty + 0.5f }, // bl
								{ ftx + 0.5f, cSE, fty + 0.5f }, // tl
								{ ftx + 0.5f, cNE, fty - 0.5f }, // tr
								{ ftx + 0.5f, fNE, fty - 0.5f }, // br
								{ 1, 0, 0 },
								{ u0, v0 }, { u0, v1 }, { u1, v1 }, { u1, v0 },
								tx, ty, static_cast<int>(FaceDir::EAST));
							break;

						case FaceDir::WEST: // x = tx - 0.5
							fNW = map.GetFloorHeightAt(tx, ty, 0);
							fSW = map.GetFloorHeightAt(tx, ty, 3);
							cNW = map.GetCeilHeightAt(tx, ty, 0);
							cSW = map.GetCeilHeightAt(tx, ty, 3);
							addQuad(
								{ ftx - 0.5f, fNW, fty - 0.5f }, // bl
								{ ftx - 0.5f, cNW, fty - 0.5f }, // tl
								{ ftx - 0.5f, cSW, fty + 0.5f }, // tr
								{ ftx - 0.5f, fSW, fty + 0.5f }, // br
								{ -1, 0, 0 },
								{ u0, v0 }, { u0, v1 }, { u1, v1 }, { u1, v0 },
								tx, ty, static_cast<int>(FaceDir::WEST));
							break;

						case FaceDir::FLOOR:
						case FaceDir::CEILING:
						case FaceDir::COUNT:
							break;
						}
					}
				};

				uint8_t wt = isHighlighted(tx, ty) ? highlightTex : tile.wallTex;
				addWallIf(tx, ty - 1, FaceDir::NORTH, wt);
				addWallIf(tx, ty + 1, FaceDir::SOUTH, wt);
				addWallIf(tx + 1, ty, FaceDir::EAST, wt);
				addWallIf(tx - 1, ty, FaceDir::WEST, wt);
			}
		}
	}

	gr::Mesh TileMeshGen::CreateMesh() const
	{
		gr::Mesh mesh;
		if (positions.empty()) return mesh;

		std::vector<gr::MeshVertex> verts;
		verts.reserve(positions.size());
		for (size_t i = 0; i < positions.size(); ++i)
		{
			verts.push_back({ positions[i], normals[i], uvs[i] });
		}

		mesh.vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);
		mesh.vbo = gpu::buffer::CreateBuffer(
			verts.data(), verts.size() * sizeof(gr::MeshVertex));
		mesh.ibo = gpu::buffer::CreateBuffer(
			indices.data(), indices.size() * sizeof(uint32_t));
		mesh.vertexCount = static_cast<uint32_t>(verts.size());
		mesh.indexCount  = static_cast<uint32_t>(indices.size());
		mesh.isIndexed   = true;

		mesh.ComputeAABB(positions);
		return mesh;
	}

	bool TileMeshGen::RayIntersect(
		glm::vec3 orig, glm::vec3 dir, HitInfo& hit) const
	{
		hit.tileX = -1;
		hit.t     = FLT_MAX;
		bool found = false;

		for (size_t i = 0; i + 2 < indices.size(); i += 3)
		{
			const auto& v0 = positions[indices[i]];
			const auto& v1 = positions[indices[i + 1]];
			const auto& v2 = positions[indices[i + 2]];

			float t, u, v;
			if (RayTriangleIntersect(orig, dir, v0, v1, v2, t, u, v))
			{
				if (t > 0 && t < hit.t)
				{
					hit.t      = t;
					hit.point  = orig + dir * t;
					hit.tileX  = triTileX[i / 3];
					hit.tileY  = triTileY[i / 3];
					hit.face   = static_cast<FaceDir>(triFace[i / 3]);
					hit.corner = -1;
					found = true;
				}
			}
		}
		return found;
	}

	// ==========================================================================
	//  Procedural texture atlas
	// ==========================================================================
	static uint8_t valueNoise(int x, int y, int seed)
	{
		uint32_t h = static_cast<uint32_t>(x * 374761393u + y * 668265263u + seed * 1274126177u);
		h = (h ^ (h >> 13)) * 1274126177u;
		h = h ^ (h >> 16);
		return static_cast<uint8_t>(h & 0xFF);
	}

	static void fillCheckerPattern(uint8_t* rgba, int w, int h, int cellSize,
		uint8_t r0, uint8_t g0, uint8_t b0,
		uint8_t r1, uint8_t g1, uint8_t b1)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * w + x) * 4;
				bool c = ((x / cellSize) + (y / cellSize)) & 1;
				if (c) { rgba[idx] = r1; rgba[idx + 1] = g1; rgba[idx + 2] = b1; }
				else   { rgba[idx] = r0; rgba[idx + 1] = g0; rgba[idx + 2] = b0; }
				rgba[idx + 3] = 255;
			}
		}
	}

	static void fillStonePattern(uint8_t* rgba, int w, int h, int seed,
		uint8_t baseR, uint8_t baseG, uint8_t baseB)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * w + x) * 4;
				uint8_t n = valueNoise(x, y, seed);
				int v = (static_cast<int>(n) - 128) / 4;
				rgba[idx]     = static_cast<uint8_t>(std::clamp(baseR + v, 0, 255));
				rgba[idx + 1] = static_cast<uint8_t>(std::clamp(baseG + v, 0, 255));
				rgba[idx + 2] = static_cast<uint8_t>(std::clamp(baseB + v, 0, 255));
				rgba[idx + 3] = 255;
			}
		}
		// mortar lines
		for (int y = 0; y < h; y += 16)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * w + x) * 4;
				rgba[idx]     = static_cast<uint8_t>(rgba[idx] * 3 / 4);
				rgba[idx + 1] = static_cast<uint8_t>(rgba[idx + 1] * 3 / 4);
				rgba[idx + 2] = static_cast<uint8_t>(rgba[idx + 2] * 3 / 4);
			}
		}
		for (int x = 0; x < w; x += 16)
		{
			for (int y = 0; y < h; ++y)
			{
				int idx = (y * w + x) * 4;
				rgba[idx]     = static_cast<uint8_t>(rgba[idx] * 3 / 4);
				rgba[idx + 1] = static_cast<uint8_t>(rgba[idx + 1] * 3 / 4);
				rgba[idx + 2] = static_cast<uint8_t>(rgba[idx + 2] * 3 / 4);
			}
		}
	}

	gpu::texture::TexturePtr CreateTileAtlas(int tileSize, int atlasDim)
	{
		uint32_t totalW = static_cast<uint32_t>(tileSize * atlasDim);
		uint32_t totalH = static_cast<uint32_t>(tileSize * atlasDim);
		std::vector<uint8_t> pixels(totalW * totalH * 4, 255);

		// tex 0 = wall (gray stone)
		{
			int ox = 0, oy = 0;
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 42, 140, 130, 120);
		}
		// tex 1 = floor (brown checker)
		{
			int ox = tileSize * 1, oy = 0;
			fillCheckerPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 8, 180, 140, 100, 160, 120, 80);
		}
		// tex 2 = ceiling (dark gray)
		{
			int ox = tileSize * 2, oy = 0;
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 99, 60, 60, 65);
		}
		// tex 3 = selected (yellow)
		{
			int ox = tileSize * 3, oy = 0;
			fillCheckerPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 4, 255, 255, 0, 200, 200, 0);
		}
		// tex 4 = red brick
		{
			int ox = 0, oy = tileSize;
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 17, 180, 60, 50);
		}
		// tex 5 = blue
		{
			int ox = tileSize * 1, oy = tileSize;
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 33, 60, 100, 180);
		}
		// tex 6 = green
		{
			int ox = tileSize * 2, oy = tileSize;
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, 55, 80, 160, 70);
		}
		// tex 7+ fill remaining with random colors
		for (int idx = 7; idx < atlasDim * atlasDim; ++idx)
		{
			int ox = (idx % atlasDim) * tileSize;
			int oy = (idx / atlasDim) * tileSize;
			uint32_t h = idx * 1640531527u;
			uint8_t r = static_cast<uint8_t>((h >> 16) & 0xFF);
			uint8_t g = static_cast<uint8_t>((h >> 8) & 0xFF);
			uint8_t b = static_cast<uint8_t>(h & 0xFF);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, idx * 37, r, g, b);
		}

		auto tex = gpu::texture::CreateTexture2D(
			{ totalW, totalH },
			gpu::Format::R8G8B8A8_UNORM,
			"tileAtlas");

		gpu::texture::TextureUpdateInfo update{};
		update.level  = 0;
		update.extent = { totalW, totalH, 1u };
		update.pixels = pixels.data();
		update.format = gpu::UploadFormat::RGBA;
		update.type   = gpu::UploadType::UBYTE;
		gpu::texture::UpdateImage(tex, update);

		return tex;
	}

	// ==========================================================================
	//  Ray-triangle intersection (Möller–Trumbore)
	// ==========================================================================
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

	// ==========================================================================
	//  Screen → World ray
	// ==========================================================================
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

} // namespace tile
