#include "stdafx.h"
#include "sc_chunkNode.h"
//=============================================================================
scene::ChunkNode::ChunkNode(std::string name_)
	: SceneNode(std::move(name_), NodeType::Chunk)
{}
//=============================================================================
void scene::ChunkNode::AddBatch(
	std::shared_ptr<gr::Mesh> mesh_,
	std::shared_ptr<gr::Material> material_,
	std::vector<glm::mat4> transforms_)
{
	m_batches.push_back({
		.mesh = std::move(mesh_),
		.material = std::move(material_),
		.transforms = std::move(transforms_)
	});
}
//=============================================================================
void scene::ChunkNode::ClearBatches()
{
	m_batches.clear();
	chunkAABB.Reset();
}
//=============================================================================
void scene::ChunkNode::RebuildAABB()
{
	chunkAABB.Reset();
	for (const auto& batch : m_batches)
	{
		if (!batch.mesh) continue;
		for (const auto& m : batch.transforms)
		{
			math::AABB worldAABB = batch.mesh->aabb.Transform(m);
			chunkAABB.Expand(worldAABB);
		}
	}
}
//=============================================================================