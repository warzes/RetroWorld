#include "stdafx.h"
#include "Editor.h"

// ---- RebuildTileMesh ----
void RebuildTileMesh()
{
	g_tileMeshCPU.BuildFromMap(g_tileMap, 8);

	if (g_hoverTX >= 0)
	{
		g_tileMeshCPU.SetFaceColor(g_hoverTX, g_hoverTY,
			static_cast<int>(g_hoverFace), HOVER_PINK);
	}

	if (g_tileModelNode)
	{
		g_tileMesh.Close();
		g_tileMesh = g_tileMeshCPU.CreateMesh();
		g_tileModelNode->mesh = std::make_shared<gr::Mesh>(g_tileMesh);
	}
	g_dirtyMesh = false;
}

// ---- PickTile ----
void PickTile(const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
	tile::HitInfo hit;
	if (g_tileMeshCPU.RayIntersect(rayOrigin, rayDir, hit))
	{
		g_selTX = hit.tileX;
		g_selTY = hit.tileY;
		g_selFace = hit.face;
		g_selCorner = -1;
		g_dirtyMesh = true;

		if (g_editMode == EditMode::VERTEX)
		{
			auto ci = g_tileMap.FindNearestCorner(hit.point, 0.4f);
			if (ci.dist < 0.4f)
			{
				g_selTX = ci.tx;
				g_selTY = ci.ty;
				g_selCorner = ci.corner;
			}
		}
	}
	else
	{
		g_selTX = g_selTY = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_selCorner = -1;
		g_dirtyMesh = true;
	}
}
