#pragma once

#include "sc_node.h"
#include "gr_mesh.h"
#include "gr_material.h"

namespace scene
{
	// A ChunkBatch represents a group of instances sharing the same mesh+material
	struct ChunkBatch final
	{
		std::shared_ptr<gr::Mesh>     mesh;
		std::shared_ptr<gr::Material> material;
		std::vector<glm::mat4>        transforms; // per-instance world matrices
	};

	// ChunkNode — a scene node representing a tile/geometry chunk.
	// Holds multiple ChunkBatches internally; the SceneManager collects
	// them during BuildRenderQueue and submits them as instanced draw calls.
	// The chunk as a whole is frustum-culled via its AABB.
	class ChunkNode final : public SceneNode
	{
	public:
		explicit ChunkNode(std::string name);

		// Add a batch (takes ownership of the shared_ptr contents)
		void AddBatch(std::shared_ptr<gr::Mesh> mesh,
			std::shared_ptr<gr::Material> material,
			std::vector<glm::mat4> transforms);

		// Clear all batches (e.g. when chunk data changes)
		void ClearBatches();

		// Rebuild the chunk AABB from all batch transforms + mesh AABBs
		void RebuildAABB();

		// Access batches for rendering
		const std::vector<ChunkBatch>& GetBatches() const { return m_batches; }

		// Whole-chunk culling AABB in local space
		math::AABB chunkAABB;

		bool castShadow = true;
		bool receiveShadow = true;

	private:
		std::vector<ChunkBatch> m_batches;
	};
} // namespace scene