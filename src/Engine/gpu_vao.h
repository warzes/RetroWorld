#pragma once

#include "gpu_core.h"

namespace gpu::buffer
{
	struct Buffer;
	using BufferPtr = std::shared_ptr<Buffer>;
} // namespace gpu::buffer

namespace gpu::vao
{
	struct VertexArray;
	using VertexArrayPtr = std::shared_ptr<VertexArray>;

	struct VertexInputBindingDescription final
	{
		uint32_t location{ 0 }; // glEnableVertexArrayAttrib + glVertexArrayAttribFormat
		uint32_t binding{ 0 };  // glVertexArrayAttribBinding
		Format   format{ 0 };   // glVertexArrayAttribFormat
		uint32_t offset{ 0 };   // glVertexArrayAttribFormat
	};

	VertexArrayPtr CreateVertexArray(const std::vector<VertexInputBindingDescription>& vertexInputState);

	[[nodiscard]] uint32_t Handle(VertexArrayPtr vao) noexcept;
	[[nodiscard]] bool IsValid(VertexArrayPtr vao) noexcept;

	void BindVertexArray(VertexArrayPtr vao);

	void BindVertexBuffer(VertexArrayPtr vao, uint32_t bindingIndex, gpu::buffer::BufferPtr buffer, uint64_t offset, uint64_t stride);
	void BindIndexBuffer(VertexArrayPtr vao, gpu::buffer::BufferPtr buffer, IndexType indexType);

} // namespace gpu::vao