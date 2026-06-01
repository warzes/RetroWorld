#pragma once

#include "TileTypes.h"
#include "TileMap.h"
#include <gr_mesh.h>

namespace tile
{
	struct TileMeshGen final
	{
		std::vector<glm::vec3>  positions;
		std::vector<glm::vec3>  normals;
		std::vector<glm::vec2>  uvs;
		std::vector<glm::vec4>  colors;
		std::vector<uint32_t>   indices;

		std::vector<int> triTileX;
		std::vector<int> triTileY;
		std::vector<int> triFace;

		void Clear();

		void BuildFromMap(const TileMap& map, int atlasDim = 4, int highlightTX = -1, int highlightTY = -1, uint8_t highlightTex = 3);

		gr::Mesh CreateMesh() const;

		bool RayIntersect(glm::vec3 orig, glm::vec3 dir, HitInfo& hit) const;

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
}
