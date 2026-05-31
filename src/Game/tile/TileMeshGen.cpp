#include "stdafx.h"
#include "TileMeshGen.h"
#include "TileMap.h"
#include <cfloat>
#include <cmath>
#include <glm/glm.hpp>

namespace tile
{
	void TileMeshGen::Clear()
	{
		positions.clear();
		normals.clear();
		uvs.clear();
		colors.clear();
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
		constexpr glm::vec4 white{1.0f};
		uint32_t base = static_cast<uint32_t>(positions.size());
		positions.push_back(a); normals.push_back(n); uvs.push_back(uv00); colors.push_back(white);
		positions.push_back(b); normals.push_back(n); uvs.push_back(uv10); colors.push_back(white);
		positions.push_back(c); normals.push_back(n); uvs.push_back(uv11); colors.push_back(white);
		positions.push_back(d); normals.push_back(n); uvs.push_back(uv01); colors.push_back(white);

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
		constexpr glm::vec4 white{1.0f};
		uint32_t base = static_cast<uint32_t>(positions.size());
		positions.push_back(a); normals.push_back(n); uvs.push_back(uvA); colors.push_back(white);
		positions.push_back(b); normals.push_back(n); uvs.push_back(uvB); colors.push_back(white);
		positions.push_back(c); normals.push_back(n); uvs.push_back(uvC); colors.push_back(white);

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
		float invRepDim = 1.0f / static_cast<float>(atlasDim * atlasDim);
		float wallVScale = static_cast<float>(atlasDim);
		float wallVBias = static_cast<float>(atlasDim) - 0.5f;
		float vTop = 1.0f - invDim * 0.001f; // slightly below 1.0 to avoid REPEAT wrapping at V=1.0

		auto isHighlighted = [&](int tx, int ty) noexcept -> bool {
			return tx == highlightTX && ty == highlightTY;
		};

		for (int ty = 0; ty < map.GetHeight(); ++ty)
		{
			for (int tx = 0; tx < map.GetWidth(); ++tx)
			{
				const Tile& tile = map.Get(tx, ty);
				if (tile.spaceType != TileSpaceType::SOLID) continue;

				// Floor (repeating atlas: column = ti, repetition 0)
				{
					int ti = isHighlighted(tx, ty) ? highlightTex : tile.floorTex;
					float u0 = static_cast<float>(ti) * invRepDim;
					float u1 = u0 + invRepDim;
					float v0 = vTop;
					float v1 = vTop - invDim;

					float ftx = static_cast<float>(tx);
					float fty = static_cast<float>(ty);
					glm::vec3 nw{ ftx - 0.5f, map.GetFloorHeightAt(tx, ty, 0), fty - 0.5f };
					glm::vec3 ne{ ftx + 0.5f, map.GetFloorHeightAt(tx, ty, 1), fty - 0.5f };
					glm::vec3 se{ ftx + 0.5f, map.GetFloorHeightAt(tx, ty, 2), fty + 0.5f };
					glm::vec3 sw{ ftx - 0.5f, map.GetFloorHeightAt(tx, ty, 3), fty + 0.5f };

					glm::vec3 n = glm::normalize(glm::cross(se - nw, ne - nw));

					addTri(nw, se, ne, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u1, v1 }, glm::vec2{ u1, v0 },
						tx, ty, static_cast<int>(FaceDir::FLOOR));

					addTri(nw, sw, se, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u0, v1 }, glm::vec2{ u1, v1 },
						tx, ty, static_cast<int>(FaceDir::FLOOR));
				}

				// Ceiling (repeating atlas: column = ti, repetition 0)
				{
					int ti = isHighlighted(tx, ty) ? highlightTex : tile.ceilTex;
					float u0 = static_cast<float>(ti) * invRepDim;
					float u1 = u0 + invRepDim;
					float v0 = vTop;
					float v1 = vTop - invDim;

					float ftx = static_cast<float>(tx);
					float fty = static_cast<float>(ty);
					glm::vec3 nw{ ftx - 0.5f, map.GetCeilHeightAt(tx, ty, 0), fty - 0.5f };
					glm::vec3 ne{ ftx + 0.5f, map.GetCeilHeightAt(tx, ty, 1), fty - 0.5f };
					glm::vec3 se{ ftx + 0.5f, map.GetCeilHeightAt(tx, ty, 2), fty + 0.5f };
					glm::vec3 sw{ ftx - 0.5f, map.GetCeilHeightAt(tx, ty, 3), fty + 0.5f };

					glm::vec3 n = glm::normalize(glm::cross(ne - nw, se - nw));

					addTri(nw, ne, se, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u1, v0 }, glm::vec2{ u1, v1 },
						tx, ty, static_cast<int>(FaceDir::CEILING));

					addTri(nw, se, sw, n,
						glm::vec2{ u0, v0 }, glm::vec2{ u1, v1 }, glm::vec2{ u0, v1 },
						tx, ty, static_cast<int>(FaceDir::CEILING));
				}

				// Walls (repeating atlas: column = ti, V = height-proportional)
				float ftx = static_cast<float>(tx);
				float fty = static_cast<float>(ty);
				auto addWallIf = [&](int ntx, int nty, FaceDir dir, uint8_t tex)
				{
					bool neighborInBounds = map.InBounds(ntx, nty);
					const Tile* neighbor = neighborInBounds ? &map.Get(ntx, nty) : nullptr;
					bool neighborIsSolid = neighbor && neighbor->spaceType == TileSpaceType::SOLID;

					float u0 = static_cast<float>(tex) * invRepDim;
					float u1 = u0 + invRepDim;

					auto wallV = [&](float worldY) noexcept -> float {
						return (-worldY + wallVBias) / wallVScale;
					};



					if (!neighborIsSolid)
					{
						// Non-solid neighbor: full wall from floor to ceiling
						float fNW = 0.0f, fNE = 0.0f, fSW = 0.0f, fSE = 0.0f;
						float cNW = 0.0f, cNE = 0.0f, cSW = 0.0f, cSE = 0.0f;

						switch (dir)
						{
						case FaceDir::NORTH:
							fNW = map.GetFloorHeightAt(tx, ty, 0);
							fNE = map.GetFloorHeightAt(tx, ty, 1);
							cNW = map.GetCeilHeightAt(tx, ty, 0);
							cNE = map.GetCeilHeightAt(tx, ty, 1);
							addQuad(
								{ ftx - 0.5f, fNW, fty - 0.5f },
								{ ftx + 0.5f, fNE, fty - 0.5f },
								{ ftx + 0.5f, cNE, fty - 0.5f },
								{ ftx - 0.5f, cNW, fty - 0.5f },
								{ 0, 0, -1 },
								{ u0, wallV(fNW) }, { u1, wallV(fNE) },
								{ u1, wallV(cNE) }, { u0, wallV(cNW) },
								tx, ty, static_cast<int>(FaceDir::NORTH));
							break;

						case FaceDir::SOUTH:
							fSW = map.GetFloorHeightAt(tx, ty, 3);
							fSE = map.GetFloorHeightAt(tx, ty, 2);
							cSW = map.GetCeilHeightAt(tx, ty, 3);
							cSE = map.GetCeilHeightAt(tx, ty, 2);
							addQuad(
								{ ftx + 0.5f, fSE, fty + 0.5f },
								{ ftx - 0.5f, fSW, fty + 0.5f },
								{ ftx - 0.5f, cSW, fty + 0.5f },
								{ ftx + 0.5f, cSE, fty + 0.5f },
								{ 0, 0, 1 },
								{ u1, wallV(fSE) }, { u0, wallV(fSW) },
								{ u0, wallV(cSW) }, { u1, wallV(cSE) },
								tx, ty, static_cast<int>(FaceDir::SOUTH));
							break;

						case FaceDir::EAST:
							fNE = map.GetFloorHeightAt(tx, ty, 1);
							fSE = map.GetFloorHeightAt(tx, ty, 2);
							cNE = map.GetCeilHeightAt(tx, ty, 1);
							cSE = map.GetCeilHeightAt(tx, ty, 2);
							addQuad(
								{ ftx + 0.5f, fSE, fty + 0.5f },
								{ ftx + 0.5f, cSE, fty + 0.5f },
								{ ftx + 0.5f, cNE, fty - 0.5f },
								{ ftx + 0.5f, fNE, fty - 0.5f },
								{ 1, 0, 0 },
								{ u1, wallV(fSE) }, { u1, wallV(cSE) },
								{ u0, wallV(cNE) }, { u0, wallV(fNE) },
								tx, ty, static_cast<int>(FaceDir::EAST));
							break;

						case FaceDir::WEST:
							fNW = map.GetFloorHeightAt(tx, ty, 0);
							fSW = map.GetFloorHeightAt(tx, ty, 3);
							cNW = map.GetCeilHeightAt(tx, ty, 0);
							cSW = map.GetCeilHeightAt(tx, ty, 3);
							addQuad(
								{ ftx - 0.5f, fNW, fty - 0.5f },
								{ ftx - 0.5f, cNW, fty - 0.5f },
								{ ftx - 0.5f, cSW, fty + 0.5f },
								{ ftx - 0.5f, fSW, fty + 0.5f },
								{ -1, 0, 0 },
								{ u0, wallV(fNW) }, { u0, wallV(cNW) },
								{ u1, wallV(cSW) }, { u1, wallV(fSW) },
								tx, ty, static_cast<int>(FaceDir::WEST));
							break;

						case FaceDir::FLOOR:
						case FaceDir::CEILING:
						case FaceDir::COUNT:
							break;
						}
					}
				else
				{
					// Solid neighbor: render only non-overlapping portions (INWARD-facing)
					float ourF[2], ourC[2], nF[2], nC[2];

					auto faceIdx = static_cast<int>(dir);
					switch (dir)
					{
					case FaceDir::NORTH:
					{
						// Our edge: corners 0 (NW = left), 1 (NE = right)
						ourF[0] = map.GetFloorHeightAt(tx, ty, 0);
						ourF[1] = map.GetFloorHeightAt(tx, ty, 1);
						ourC[0] = map.GetCeilHeightAt(tx, ty, 0);
						ourC[1] = map.GetCeilHeightAt(tx, ty, 1);
						// Neighbor's shared edge: corners 3 (SW), 2 (SE)
						nF[0] = map.GetFloorHeightAt(ntx, nty, 3);
						nF[1] = map.GetFloorHeightAt(ntx, nty, 2);
						nC[0] = map.GetCeilHeightAt(ntx, nty, 3);
						nC[1] = map.GetCeilHeightAt(ntx, nty, 2);

						auto add = [&](float b0, float b1, float t0, float t1)
						{
							float hBL = std::min(b0, t0);
							float hBR = std::min(b1, t1);
							float hTL = std::max(b0, t0);
							float hTR = std::max(b1, t1);
							if (hBL >= hTL && hBR >= hTR) return;
							glm::vec3 pL{ ftx - 0.5f, 0, fty - 0.5f };
							glm::vec3 pR{ ftx + 0.5f, 0, fty - 0.5f };
							addQuad(
								{ pL.x, hTL, pL.z }, { pR.x, hTR, pR.z },
								{ pR.x, hBR, pR.z }, { pL.x, hBL, pL.z },
								{ 0, 0, -1 },
								{ u0, wallV(hTL) }, { u1, wallV(hTR) },
								{ u1, wallV(hBR) }, { u0, wallV(hBL) },
								tx, ty, faceIdx);
						};

						add(ourF[0], ourF[1], std::min(nF[0], ourC[0]), std::min(nF[1], ourC[1]));
						add(std::max(nC[0], ourF[0]), std::max(nC[1], ourF[1]), ourC[0], ourC[1]);
						break;
					}
					case FaceDir::SOUTH:
					{
						// Our edge: corners 3 (SW = left), 2 (SE = right)
						ourF[0] = map.GetFloorHeightAt(tx, ty, 3);
						ourF[1] = map.GetFloorHeightAt(tx, ty, 2);
						ourC[0] = map.GetCeilHeightAt(tx, ty, 3);
						ourC[1] = map.GetCeilHeightAt(tx, ty, 2);
						// Neighbor's shared edge: corners 0 (NW), 1 (NE)
						nF[0] = map.GetFloorHeightAt(ntx, nty, 0);
						nF[1] = map.GetFloorHeightAt(ntx, nty, 1);
						nC[0] = map.GetCeilHeightAt(ntx, nty, 0);
						nC[1] = map.GetCeilHeightAt(ntx, nty, 1);

						auto add = [&](float b0, float b1, float t0, float t1)
						{
							float hBL = std::min(b0, t0);
							float hBR = std::min(b1, t1);
							float hTL = std::max(b0, t0);
							float hTR = std::max(b1, t1);
							if (hBL >= hTL && hBR >= hTR) return;
							glm::vec3 pL{ ftx - 0.5f, 0, fty + 0.5f };
							glm::vec3 pR{ ftx + 0.5f, 0, fty + 0.5f };
							addQuad(
								{ pR.x, hTR, pR.z }, { pL.x, hTL, pL.z },
								{ pL.x, hBL, pL.z }, { pR.x, hBR, pR.z },
								{ 0, 0, 1 },
								{ u1, wallV(hTR) }, { u0, wallV(hTL) },
								{ u0, wallV(hBL) }, { u1, wallV(hBR) },
								tx, ty, faceIdx);
						};

						add(ourF[0], ourF[1], std::min(nF[0], ourC[0]), std::min(nF[1], ourC[1]));
						add(std::max(nC[0], ourF[0]), std::max(nC[1], ourF[1]), ourC[0], ourC[1]);
						break;
					}
					case FaceDir::EAST:
					{
						// Our edge: corners 1 (NE = left), 2 (SE = right)
						ourF[0] = map.GetFloorHeightAt(tx, ty, 1);
						ourF[1] = map.GetFloorHeightAt(tx, ty, 2);
						ourC[0] = map.GetCeilHeightAt(tx, ty, 1);
						ourC[1] = map.GetCeilHeightAt(tx, ty, 2);
						// Neighbor's shared edge: corners 0 (NW), 3 (SW)
						nF[0] = map.GetFloorHeightAt(ntx, nty, 0);
						nF[1] = map.GetFloorHeightAt(ntx, nty, 3);
						nC[0] = map.GetCeilHeightAt(ntx, nty, 0);
						nC[1] = map.GetCeilHeightAt(ntx, nty, 3);

						auto add = [&](float b0, float b1, float t0, float t1)
						{
							float hBL = std::min(b0, t0);
							float hBR = std::min(b1, t1);
							float hTL = std::max(b0, t0);
							float hTR = std::max(b1, t1);
							if (hBL >= hTL && hBR >= hTR) return;
							glm::vec3 pL{ ftx + 0.5f, 0, fty - 0.5f };
							glm::vec3 pR{ ftx + 0.5f, 0, fty + 0.5f };
							addQuad(
								{ pL.x, hBL, pL.z }, { pL.x, hTL, pL.z },
								{ pR.x, hTR, pR.z }, { pR.x, hBR, pR.z },
								{ 1, 0, 0 },
								{ u0, wallV(hBL) }, { u0, wallV(hTL) },
								{ u1, wallV(hTR) }, { u1, wallV(hBR) },
								tx, ty, faceIdx);
						};

						add(ourF[0], ourF[1], std::min(nF[0], ourC[0]), std::min(nF[1], ourC[1]));
						add(std::max(nC[0], ourF[0]), std::max(nC[1], ourF[1]), ourC[0], ourC[1]);
						break;
					}
					case FaceDir::WEST:
					{
						// Our edge: corners 0 (NW = left), 3 (SW = right)
						ourF[0] = map.GetFloorHeightAt(tx, ty, 0);
						ourF[1] = map.GetFloorHeightAt(tx, ty, 3);
						ourC[0] = map.GetCeilHeightAt(tx, ty, 0);
						ourC[1] = map.GetCeilHeightAt(tx, ty, 3);
						// Neighbor's shared edge: corners 1 (NE), 2 (SE)
						nF[0] = map.GetFloorHeightAt(ntx, nty, 1);
						nF[1] = map.GetFloorHeightAt(ntx, nty, 2);
						nC[0] = map.GetCeilHeightAt(ntx, nty, 1);
						nC[1] = map.GetCeilHeightAt(ntx, nty, 2);

						auto add = [&](float b0, float b1, float t0, float t1)
						{
							float hBL = std::min(b0, t0);
							float hBR = std::min(b1, t1);
							float hTL = std::max(b0, t0);
							float hTR = std::max(b1, t1);
							if (hBL >= hTL && hBR >= hTR) return;
							glm::vec3 pL{ ftx - 0.5f, 0, fty - 0.5f };
							glm::vec3 pR{ ftx - 0.5f, 0, fty + 0.5f };
							addQuad(
								{ pR.x, hBR, pR.z }, { pR.x, hTR, pR.z },
								{ pL.x, hTL, pL.z }, { pL.x, hBL, pL.z },
								{ -1, 0, 0 },
								{ u1, wallV(hBR) }, { u1, wallV(hTR) },
								{ u0, wallV(hTL) }, { u0, wallV(hBL) },
								tx, ty, faceIdx);
						};

						add(ourF[0], ourF[1], std::min(nF[0], ourC[0]), std::min(nF[1], ourC[1]));
						add(std::max(nC[0], ourF[0]), std::max(nC[1], ourF[1]), ourC[0], ourC[1]);
						break;
					}
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
			verts.push_back({ positions[i], normals[i], uvs[i], colors[i] });
		}

		mesh.vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);
		mesh.vbo = gpu::buffer::CreateBuffer(
			verts.data(), verts.size() * sizeof(gr::MeshVertex),
			gpu::buffer::BufferStorageFlag::DynamicStorage);
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

			constexpr float EPS = 1e-8f;
			glm::vec3 e1 = v1 - v0;
			glm::vec3 e2 = v2 - v0;
			glm::vec3 pvec = glm::cross(dir, e2);
			float det = glm::dot(e1, pvec);
			if (det <= EPS) continue;
			float invDet = 1.0f / det;
			glm::vec3 tvec = orig - v0;
			float u = glm::dot(tvec, pvec) * invDet;
			if (u < 0.0f || u > 1.0f) continue;
			glm::vec3 qvec = glm::cross(tvec, e1);
			float v = glm::dot(dir, qvec) * invDet;
			if (v < 0.0f || u + v > 1.0f) continue;
			float t = glm::dot(e2, qvec) * invDet;
			if (t <= 0.0f) continue;

			if (t < hit.t)
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
		return found;
	}

	void TileMeshGen::SetFaceColor(int tx, int ty, int faceIdx, const glm::vec4& color)
	{
		for (size_t i = 0; i < triTileX.size(); ++i)
		{
			if (triTileX[i] == tx && triTileY[i] == ty && triFace[i] == faceIdx)
			{
				uint32_t i0 = indices[i * 3];
				uint32_t i1 = indices[i * 3 + 1];
				uint32_t i2 = indices[i * 3 + 2];
				colors[i0] = color;
				colors[i1] = color;
				colors[i2] = color;
			}
		}
	}
}
