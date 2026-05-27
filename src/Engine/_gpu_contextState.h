#pragma once

#include "gpu_core.h"
#include "gpu_texture.h"

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

namespace gpu::texture
{
	struct Sampler;
	using SamplerPtr = std::shared_ptr<Sampler>;

	struct SamplerState;
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

		PrimitiveTopology currentTopology{ PrimitiveTopology::TriangleList };

		bool initViewport = true;
		Viewport lastViewport = {};
		Scissor lastScissor = {};
		bool scissorEnabled = false;

		IndexType currentIndexType{};

		std::unordered_map<size_t, gpu::vao::VertexArrayPtr> vertexArrayCache;
		std::unordered_map<gpu::texture::SamplerState, gpu::texture::SamplerPtr> samplerCache;
	} inline context;
} // namespace gpu