#pragma once

#include "gpu_core.h"
#include "gpu_texture.h"
#include "gpu_pipeline.h"

namespace gpu::program
{
	struct ShaderProgram;
	using ShaderProgramPtr = std::shared_ptr<ShaderProgram>;
} //namespace gpu::program

namespace gpu::vao
{
	struct VertexArray;
	using VertexArrayPtr = std::shared_ptr<VertexArray>;
} // namespace gpu::vao

namespace gpu::texture
{
	struct Sampler;
	using SamplerPtr = std::shared_ptr<Sampler>;

	struct SamplerState;
} // namespace gpu::texture

namespace gpu::fbo
{
	struct Framebuffer;
	using FramebufferPtr = std::shared_ptr<Framebuffer>;
} // namespace gpu::fbo

namespace gpu::detail
{
	struct TextureProxy final
	{
		bool operator==(const TextureProxy&) const noexcept = default;

		texture::TextureCreateInfo createInfo;
		uint32_t id;
	};

	struct RenderAttachments final
	{
		bool operator==(const RenderAttachments& rhs) const;

		std::vector<TextureProxy> colorAttachments{};
		std::optional<TextureProxy> depthAttachment{};
		std::optional<TextureProxy> stencilAttachment{};
	};
} // namespace gpu::detail

namespace gpu
{
	constexpr int MAX_COLOR_ATTACHMENTS = 8;

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

		bool isRenderingToSwapchain = true;

		// Used for error checking for indexed draws
		bool isIndexBufferBound = false;

		InputAssemblyState inputAssemblyState{};
		TessellationState  tessellationState{};
		RasterizationState rasterizationState{};
		MultisampleState   multisampleState{};
		DepthState         depthState{};
		StencilState       stencilState{};
		ColorBlendState    colorBlendState{};

		std::array<ColorComponentFlags, MAX_COLOR_ATTACHMENTS> lastColorMask = {};
		bool lastDepthMask = true;
		uint32_t lastStencilMask[2] = { static_cast<uint32_t>(-1), static_cast<uint32_t>(-1) };
		bool initViewport = true;
		Viewport lastViewport = {};
		Scissor lastScissor = {};
		bool scissorEnabled = false;

		IndexType currentIndexType{};

		std::unordered_map<size_t, gpu::vao::VertexArrayPtr> vertexArrayCache;
		std::unordered_map<gpu::texture::SamplerState, gpu::texture::SamplerPtr> samplerCache;

		std::vector<detail::RenderAttachments> framebufferCacheKey;
		std::vector<fbo::FramebufferPtr> framebufferCacheValue;
	} inline context;
} // namespace gpu