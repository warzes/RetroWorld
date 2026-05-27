#pragma once

#include "gpu_core.h"

namespace gpu::program
{
	struct ShaderProgram;
	using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;
}

namespace gpu::vao
{
	struct VertexArray;
	using VertexArrayPtr = std::shared_ptr<VertexArray>;
}

namespace gpu
{
	struct ContextState final
	{
		void BeginFrame();
		void EndFrame();
		void Clear();

		uint16_t contextWidth{ 0 };
		uint16_t contextHeight{ 0 };
		bool     isContextResize{ false };

		gpu::vao::VertexArrayPtr currentVertexArray{ nullptr };
		gpu::program::ShaderProgramPtr currentShaderProgram{ nullptr };

		// Used for scope error checking
		bool isRendering = false;

		// Used for error checking for indexed draws
		bool isIndexBufferBound = false;
		IndexType currentIndexType{};

		std::unordered_map<size_t, gpu::vao::VertexArrayPtr> vertexArrayCache;
	} inline context;
} // namespace gpu