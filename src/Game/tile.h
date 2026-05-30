#pragma once

#include <cstdint>
#include <vector>
#include <array>
#include <string_view>
#include <glm/glm.hpp>

#include <gr_mesh.h>
#include <gr_material.h>
#include <gpu_texture.h>

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

	// ------------------------------------------------------------------------
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
	};

	// ------------------------------------------------------------------------
	struct HitInfo final
	{
		int tileX = -1, tileY = -1;
		FaceDir face = FaceDir::COUNT;
		int corner = -1; // 0=NW, 1=NE, 2=SE, 3=SW, -1=face center
		float t = 0.0f;
		glm::vec3 point{};
	};

	// ------------------------------------------------------------------------
	struct CornerInfo final
	{
		int tx{}, ty{};
		int corner{}; // 0=NW, 1=NE, 2=SE, 3=SW
		glm::vec3 worldPos{};
		float dist{};
	};

	// ------------------------------------------------------------------------
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

		// Find nearest corner to a world position
		CornerInfo FindNearestCorner(const glm::vec3& worldPos, float threshold = 0.3f) const;

	private:
		int width  = 16;
		int height = 16;
		std::vector<Tile> tiles;

		void carveRoom(int x, int y, int w, int h);
		void carveCorridor(int x1, int y1, int x2, int y2);
	};

	// ========================================================================
	//  Mesh generation  —  CPU geometry for picking + GPU mesh creation
	// ========================================================================
	struct TileMeshGen final
	{
		std::vector<glm::vec3>  positions;
		std::vector<glm::vec3>  normals;
		std::vector<glm::vec2>  uvs;
		std::vector<glm::vec4>  colors;
		std::vector<uint32_t>   indices;

		// Per-triangle metadata  (1 entry per triangle = indices.size()/3)
		std::vector<int> triTileX;
		std::vector<int> triTileY;
		std::vector<int> triFace;

		void Clear();

		void BuildFromMap(const TileMap& map, int atlasDim = 4,
			int highlightTX = -1, int highlightTY = -1, uint8_t highlightTex = 3);

		gr::Mesh CreateMesh() const;

		// Ray intersection   —  returns hit info for the closest triangle
		bool RayIntersect(glm::vec3 orig, glm::vec3 dir, HitInfo& hit) const;

		// Set vertex color for all triangles belonging to a specific face
		void SetFaceColor(int tx, int ty, int faceIdx, const glm::vec4& color);

	private:
		void addQuad(
			glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d,
			glm::vec3 n,
			glm::vec2 uv00, glm::vec2 uv10, glm::vec2 uv11, glm::vec2 uv01,
			int tx, int ty, int faceIdx);

		void addTri(
			glm::vec3 a, glm::vec3 b, glm::vec3 c,
			glm::vec3 n,
			glm::vec2 uvA, glm::vec2 uvB, glm::vec2 uvC,
			int tx, int ty, int faceIdx);
	};

	// ========================================================================
	//  Procedural texture atlas
	// ========================================================================
	gpu::texture::TexturePtr CreateTileAtlas(int tileSize = 64, int atlasDim = 4);

	// ========================================================================
	//  Utility functions
	// ========================================================================
	bool RayTriangleIntersect(
		glm::vec3 orig, glm::vec3 dir,
		glm::vec3 v0, glm::vec3 v1, glm::vec3 v2,
		float& t, float& u, float& v);

	glm::vec3 ScreenToRay(
		const glm::mat4& viewProj,
		float mouseX, float mouseY,
		float winW, float winH);

	// Corner vertex position in local tile coords  (Y-up)
	inline glm::vec3 CornerLocalPos(int corner, float height, float slope)
	{
		float x = (corner == 1 || corner == 2) ?  0.5f : -0.5f;
		float z = (corner == 2 || corner == 3) ?  0.5f : -0.5f;
		return { x, height + slope, z };
	}

} // namespace tile
