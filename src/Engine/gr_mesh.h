#pragma once

#include "gpu_vao.h"
#include "gpu_buffer.h"
#include "math_aabb.h"

namespace gr
{
	struct MeshVertex final
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	static inline const std::vector MeshVertexBindingDescs = {
		gpu::vao::VertexInputBindingDescription{
			// position
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = offsetof(MeshVertex, position),
		},
		gpu::vao::VertexInputBindingDescription{
			// normal
			.location = 1,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = offsetof(MeshVertex, normal),
		},
		gpu::vao::VertexInputBindingDescription{
			// texcoord
			.location = 2,
			.binding  = 0,
			.format   = gpu::Format::R32G32_FLOAT,
			.offset   = offsetof(MeshVertex, uv),
		},
	};

	class Mesh final
	{
	public:
		void Close();
		void Bind() const;

		void Draw() const;
		void DrawInstanced(uint32_t count) const;

		static Mesh CreateQuad();
		static Mesh CreateCube();
		static Mesh CreatePlane(float size = 10.0f);
		static Mesh CreateSphere(int rings, int sectors);

		// Compute AABB from an array of positions (stores result in mesh.aabb)
		void ComputeAABB(std::span<const glm::vec3> positions);

		gpu::vao::VertexArrayPtr vao;
		gpu::buffer::BufferPtr   vbo;
		gpu::buffer::BufferPtr   ibo;

		uint32_t                 indexCount = 0;
		uint32_t                 vertexCount = 0;
		bool                     isIndexed = false;

		math::AABB               aabb;
	};
} // namespace gr