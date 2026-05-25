#pragma once

#include "gpu_core.h"

namespace gpu::vao
{
	struct VertexArray;
	using VertexArrayPtr = std::shared_ptr<VertexArray>;

	struct VertexAttrib final
	{
		uint32_t         index;
		int32_t          size;
		VertexAttribType type{ VertexAttribType::Float };
		bool             normalized{ false };
		uint32_t         relativeOffset;
		uint32_t         binding;
	};

	VertexArrayPtr CreateVertexArray();

	void BindVertexArray(VertexArrayPtr vao);

} // namespace gpu::vao