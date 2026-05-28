#include "stdafx.h"
#include "gr_mesh.h"
#include "gpu_cmd.h"
//=============================================================================
void gr::Mesh::Close()
{
	vao.reset();
	vbo.reset();
	ibo.reset();
}
//=============================================================================
void gr::Mesh::Bind() const
{
	if (!vao) return;
	gpu::cmd::BindVertexArray(vao);
	gpu::cmd::BindVertexBuffer(vao, 0, vbo, 0, sizeof(MeshVertex));
	if (isIndexed && ibo)
		gpu::cmd::BindIndexBuffer(vao, ibo, gpu::IndexType::UnsignedInt);
}
//=============================================================================
void gr::Mesh::Draw() const
{
	if (!vao) return;
	if (isIndexed) gpu::cmd::DrawIndexed(indexCount, 1, 0, 0, 0);
	else           gpu::cmd::Draw(vertexCount, 1, 0, 0);
}
//=============================================================================
void gr::Mesh::DrawInstanced(uint32_t count) const
{
	if (!vao || count == 0) return;
	if (isIndexed) gpu::cmd::DrawIndexed(indexCount, count, 0, 0, 0);
	else           gpu::cmd::Draw(vertexCount, count, 0, 0);
}
//=============================================================================
gr::Mesh gr::Mesh::CreateQuad()
{
	Mesh mesh;

	// Quad in XY plane, normal = +Z, UV (0,0)-(1,1)
	const std::array<MeshVertex, 4> vertices = { {
		{.position = {-0.5f, -0.5f, 0.0f}, .normal = {0, 0, 1}, .uv = {0, 0} },
		{.position = { 0.5f, -0.5f, 0.0f}, .normal = {0, 0, 1}, .uv = {1, 0} },
		{.position = { 0.5f,  0.5f, 0.0f}, .normal = {0, 0, 1}, .uv = {1, 1} },
		{.position = {-0.5f,  0.5f, 0.0f}, .normal = {0, 0, 1}, .uv = {0, 1} },
		} };
	const std::array<uint32_t, 6> indices = { 0, 1, 2, 2, 3, 0 };

	mesh.vao = gpu::vao::CreateVertexArray(MeshVertexBindingDescs);
	mesh.vbo = gpu::buffer::CreateBuffer(vertices.data(), sizeof(vertices));
	mesh.ibo = gpu::buffer::CreateBuffer(indices.data(), sizeof(indices));
	mesh.vertexCount = static_cast<uint32_t>(vertices.size());
	mesh.indexCount = static_cast<uint32_t>(indices.size());
	mesh.isIndexed = true;

	std::array<glm::vec3, 4> positions;
	for (size_t i = 0; i < 4; ++i)
		positions[i] = vertices[i].position;
	mesh.ComputeAABB(positions);

	return mesh;
}
//=============================================================================
gr::Mesh gr::Mesh::CreateCube()
{
	Mesh mesh;

	const std::array<MeshVertex, 24> vertices = { {
		// front (+z)
		{{-0.5f, -0.5f,  0.5f}, { 0,  0,  1}, {0, 0}},
		{{ 0.5f, -0.5f,  0.5f}, { 0,  0,  1}, {1, 0}},
		{{ 0.5f,  0.5f,  0.5f}, { 0,  0,  1}, {1, 1}},
		{{-0.5f,  0.5f,  0.5f}, { 0,  0,  1}, {0, 1}},
		// back (-z)
		{{-0.5f,  0.5f, -0.5f}, { 0,  0, -1}, {1, 1}},
		{{ 0.5f,  0.5f, -0.5f}, { 0,  0, -1}, {0, 1}},
		{{ 0.5f, -0.5f, -0.5f}, { 0,  0, -1}, {0, 0}},
		{{-0.5f, -0.5f, -0.5f}, { 0,  0, -1}, {1, 0}},
		// left (-x)
		{{-0.5f, -0.5f, -0.5f}, {-1,  0,  0}, {0, 0}},
		{{-0.5f, -0.5f,  0.5f}, {-1,  0,  0}, {1, 0}},
		{{-0.5f,  0.5f,  0.5f}, {-1,  0,  0}, {1, 1}},
		{{-0.5f,  0.5f, -0.5f}, {-1,  0,  0}, {0, 1}},
		// right (+x)
		{{ 0.5f,  0.5f, -0.5f}, { 1,  0,  0}, {1, 1}},
		{{ 0.5f,  0.5f,  0.5f}, { 1,  0,  0}, {0, 1}},
		{{ 0.5f, -0.5f,  0.5f}, { 1,  0,  0}, {0, 0}},
		{{ 0.5f, -0.5f, -0.5f}, { 1,  0,  0}, {1, 0}},
		// top (+y)
		{{-0.5f,  0.5f,  0.5f}, { 0,  1,  0}, {0, 0}},
		{{ 0.5f,  0.5f,  0.5f}, { 0,  1,  0}, {1, 0}},
		{{ 0.5f,  0.5f, -0.5f}, { 0,  1,  0}, {1, 1}},
		{{-0.5f,  0.5f, -0.5f}, { 0,  1,  0}, {0, 1}},
		// bottom (-y)
		{{-0.5f, -0.5f, -0.5f}, { 0, -1,  0}, {0, 0}},
		{{ 0.5f, -0.5f, -0.5f}, { 0, -1,  0}, {1, 0}},
		{{ 0.5f, -0.5f,  0.5f}, { 0, -1,  0}, {1, 1}},
		{{-0.5f, -0.5f,  0.5f}, { 0, -1,  0}, {0, 1}},
		} };

	const std::array<uint32_t, 36> indices = {
	0,  1,  2,  2,  3,  0,
	4,  5,  6,  6,  7,  4,
	8,  9, 10, 10, 11,  8,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	20, 21, 22, 22, 23, 20,
	};

	mesh.vao = gpu::vao::CreateVertexArray(MeshVertexBindingDescs);
	mesh.vbo = gpu::buffer::CreateBuffer(vertices.data(), sizeof(vertices));
	mesh.ibo = gpu::buffer::CreateBuffer(indices.data(), sizeof(indices));
	mesh.vertexCount = static_cast<uint32_t>(vertices.size());
	mesh.indexCount = static_cast<uint32_t>(indices.size());
	mesh.isIndexed = true;

	std::array<glm::vec3, 8> positions;
	for (size_t i = 0; i < 8; ++i)
	{
		float x = (i & 1) ? 0.5f : -0.5f;
		float y = (i & 2) ? 0.5f : -0.5f;
		float z = (i & 4) ? 0.5f : -0.5f;
		positions[i] = glm::vec3(x, y, z);
	}
	mesh.ComputeAABB(positions);

	return mesh;
}
//=============================================================================
gr::Mesh gr::Mesh::CreatePlane(float size)
{
	Mesh mesh;

	const float h = size * 0.5f;
	const std::array<MeshVertex, 4> vertices = { {
		{.position = {-h, 0.0f, -h}, .normal = {0, 1, 0}, .uv = {0,      0} },
		{.position = { h, 0.0f, -h}, .normal = {0, 1, 0}, .uv = {size,   0} },
		{.position = { h, 0.0f,  h}, .normal = {0, 1, 0}, .uv = {size, size} },
		{.position = {-h, 0.0f,  h}, .normal = {0, 1, 0}, .uv = {0,    size} },
	} };
	const std::array<uint32_t, 6> indices = { 1, 0, 3, 3, 2, 1 };

	mesh.vao = gpu::vao::CreateVertexArray(MeshVertexBindingDescs);
	mesh.vbo = gpu::buffer::CreateBuffer(vertices.data(), sizeof(vertices));
	mesh.ibo = gpu::buffer::CreateBuffer(indices.data(), sizeof(indices));
	mesh.vertexCount = static_cast<uint32_t>(vertices.size());
	mesh.indexCount = static_cast<uint32_t>(indices.size());
	mesh.isIndexed = true;

	std::array<glm::vec3, 4> positions;
	for (size_t i = 0; i < 4; ++i)
		positions[i] = vertices[i].position;
	mesh.ComputeAABB(positions);

	return mesh;
}
//=============================================================================
gr::Mesh gr::Mesh::CreateSphere(int rings, int sectors)
{
	Mesh mesh;

	std::vector<MeshVertex> vertices;
	vertices.reserve((rings + 1) * (sectors + 1));

	for (int r = 0; r <= rings; ++r)
	{
		float phi = glm::pi<float>() * static_cast<float>(r) / static_cast<float>(rings);
		for (int s = 0; s <= sectors; ++s)
		{
			float theta = 2.0f * glm::pi<float>() * static_cast<float>(s) / static_cast<float>(sectors);
			float sinPhi = sin(phi);
			float cosPhi = cos(phi);
			float sinTheta = sin(theta);
			float cosTheta = cos(theta);

			MeshVertex v;
			v.position = glm::vec3(sinPhi * cosTheta, cosPhi, sinPhi * sinTheta);
			v.normal = v.position;
			v.uv = glm::vec2(static_cast<float>(s) / sectors,
				static_cast<float>(r) / rings);
			vertices.push_back(v);
		}
	}

	std::vector<uint32_t> indices;
	indices.reserve(rings * sectors * 6);
	for (int r = 0; r < rings; ++r)
	{
		for (int s = 0; s < sectors; ++s)
		{
			int i0 = r * (sectors + 1) + s;
			int i1 = r * (sectors + 1) + (s + 1);
			int i2 = (r + 1) * (sectors + 1) + s;
			int i3 = (r + 1) * (sectors + 1) + (s + 1);

			indices.push_back(i0);
			indices.push_back(i1);
			indices.push_back(i2);
			indices.push_back(i2);
			indices.push_back(i1);
			indices.push_back(i3);
		}
	}

	mesh.vao = gpu::vao::CreateVertexArray(MeshVertexBindingDescs);
	mesh.vbo = gpu::buffer::CreateBuffer(vertices.data(),
		vertices.size() * sizeof(MeshVertex));
	mesh.ibo = gpu::buffer::CreateBuffer(indices.data(),
		indices.size() * sizeof(uint32_t));
	mesh.vertexCount = static_cast<uint32_t>(vertices.size());
	mesh.indexCount = static_cast<uint32_t>(indices.size());
	mesh.isIndexed = true;

	std::vector<glm::vec3> positions(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i)
		positions[i] = vertices[i].position;
	mesh.ComputeAABB(positions);

	return mesh;
}
//=============================================================================
void gr::Mesh::ComputeAABB(std::span<const glm::vec3> positions)
{
	aabb = math::AABB();
	for (const auto& p : positions)
		aabb.Expand(p);
}
//=============================================================================