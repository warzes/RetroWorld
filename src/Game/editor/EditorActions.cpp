#include "stdafx.h"
#include "Editor.h"

// ---- UploadTileMeshToGPU (persistent buffers, no Close/Create cycle) ----
static void UploadTileMeshToGPU()
{
	if (!g_tileModelNode) return;

	// Convert SoA to AoS for GPU upload
	std::vector<gr::MeshVertex> verts;
	size_t nv = g_tileMeshCPU.positions.size();
	verts.reserve(nv);
	for (size_t i = 0; i < nv; ++i)
	{
		verts.push_back({ g_tileMeshCPU.positions[i], g_tileMeshCPU.normals[i],
			g_tileMeshCPU.uvs[i], g_tileMeshCPU.colors[i] });
	}

	size_t vertsBytes = verts.size() * sizeof(gr::MeshVertex);
	size_t idxBytes   = g_tileMeshCPU.indices.size() * sizeof(uint32_t);

	// Create VAO once
	if (!g_tileMesh.vao)
		g_tileMesh.vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);

	// Create or update VBO
	if (!g_tileMesh.vbo || gpu::buffer::Size(g_tileMesh.vbo) < vertsBytes)
	{
		g_tileMesh.vbo = gpu::buffer::CreateBuffer(
			verts.data(), vertsBytes,
			gpu::buffer::BufferStorageFlag::DynamicStorage, "tile_vbo");
	}
	else
	{
		gpu::buffer::UpdateData(g_tileMesh.vbo, verts.data(), vertsBytes, 0);
	}

	// Create or update IBO
	if (!g_tileMesh.ibo || gpu::buffer::Size(g_tileMesh.ibo) < idxBytes)
	{
		g_tileMesh.ibo = gpu::buffer::CreateBuffer(
			g_tileMeshCPU.indices.data(), idxBytes,
			gpu::buffer::BufferStorageFlag::DynamicStorage, "tile_ibo");
	}
	else
	{
		gpu::buffer::UpdateData(g_tileMesh.ibo,
			g_tileMeshCPU.indices.data(), idxBytes, 0);
	}

	g_tileMesh.vertexCount = static_cast<uint32_t>(g_tileMeshCPU.positions.size());
	g_tileMesh.indexCount  = static_cast<uint32_t>(g_tileMeshCPU.indices.size());
	g_tileMesh.isIndexed   = true;
	g_tileMesh.ComputeAABB(g_tileMeshCPU.positions);
	g_tileModelNode->mesh = std::make_shared<gr::Mesh>(g_tileMesh);
}

// ---- RebuildTileMesh (full rebuild: tile data changed) ----
void RebuildTileMesh()
{
	g_tileMeshCPU.BuildFromMap(g_tileMap, 8);
	g_tileCleanColors = g_tileMeshCPU.colors; // save clean copy

	if (g_hoverTX >= 0)
	{
		g_tileMeshCPU.SetFaceColor(g_hoverTX, g_hoverTY,
			static_cast<int>(g_hoverFace), HOVER_PINK);
	}

	UploadTileMeshToGPU();
	g_dirtyMesh = false;

	// Rebuild physics collider for the updated tile mesh
	RebuildMapCollider();
}

// ---- UpdateHoverHighlight (lightweight: only vertex colors) ----
void UpdateHoverHighlight()
{
	if (!g_tileModelNode || !g_tileMesh.vbo) return;
	if (g_tileCleanColors.size() != g_tileMeshCPU.colors.size()) return;

	// g_prevHover* still holds the PREVIOUS frame's hover (old hover to restore)

	// Restore previous hover face to clean color
	auto restoreFace = [&](int tx, int ty, tile::FaceDir face)
	{
		for (size_t i = 0; i < g_tileMeshCPU.triTileX.size(); ++i)
		{
			if (g_tileMeshCPU.triTileX[i] == tx &&
				g_tileMeshCPU.triTileY[i] == ty &&
				g_tileMeshCPU.triFace[i] == static_cast<int>(face))
			{
				uint32_t i0 = g_tileMeshCPU.indices[i * 3];
				uint32_t i1 = g_tileMeshCPU.indices[i * 3 + 1];
				uint32_t i2 = g_tileMeshCPU.indices[i * 3 + 2];
				g_tileMeshCPU.colors[i0] = g_tileCleanColors[i0];
				g_tileMeshCPU.colors[i1] = g_tileCleanColors[i1];
				g_tileMeshCPU.colors[i2] = g_tileCleanColors[i2];
			}
		}
	};

	if (g_prevHoverTX >= 0 && g_prevHoverFace != tile::FaceDir::COUNT)
		restoreFace(g_prevHoverTX, g_prevHoverTY, g_prevHoverFace);

	// Apply pink to current hover face
	if (g_hoverTX >= 0 && g_hoverFace != tile::FaceDir::COUNT)
	{
		g_tileMeshCPU.SetFaceColor(g_hoverTX, g_hoverTY,
			static_cast<int>(g_hoverFace), HOVER_PINK);
	}

	// Update prev for next frame
	g_prevHoverTX = g_hoverTX;
	g_prevHoverTY = g_hoverTY;
	g_prevHoverFace = g_hoverFace;

	// Upload all vertex data to GPU (single coherent UpdateData call)
	std::vector<gr::MeshVertex> verts;
	size_t nv = g_tileMeshCPU.positions.size();
	verts.reserve(nv);
	for (size_t i = 0; i < nv; ++i)
	{
		verts.push_back({ g_tileMeshCPU.positions[i], g_tileMeshCPU.normals[i],
			g_tileMeshCPU.uvs[i], g_tileMeshCPU.colors[i] });
	}
	gpu::buffer::UpdateData(g_tileMesh.vbo, verts.data(),
		verts.size() * sizeof(gr::MeshVertex), 0);

	g_hoverDirty = false;
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
	else if (fabsf(rayDir.y) > 1e-6f)
	{
		// Ground-plane fallback: pick tile at Y=0 intersection
		float t = -rayOrigin.y / rayDir.y;
		if (t > 0)
		{
			glm::vec3 hp = rayOrigin + rayDir * t;
			int tx = static_cast<int>(floor(hp.x + 0.5f));
			int ty = static_cast<int>(floor(hp.z + 0.5f));
			if (g_tileMap.InBounds(tx, ty))
			{
				g_selTX = tx;
				g_selTY = ty;
				g_selFace = tile::FaceDir::COUNT;
				g_selCorner = -1;
				g_dirtyMesh = true;
				return;
			}
		}
		g_selTX = g_selTY = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_selCorner = -1;
		g_dirtyMesh = true;
	}
	else
	{
		g_selTX = g_selTY = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_selCorner = -1;
		g_dirtyMesh = true;
	}
}
