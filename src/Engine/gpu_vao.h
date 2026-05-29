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

	[[nodiscard]] VertexArrayPtr CreateVertexArray(const std::vector<VertexInputBindingDescription>& vertexInputState);

	[[nodiscard]] uint32_t Handle(const VertexArrayPtr& vao) noexcept;
	[[nodiscard]] bool IsValid(const VertexArrayPtr& vao) noexcept;

	void BindVertexArray(const VertexArrayPtr& vao);

} // namespace gpu::vao