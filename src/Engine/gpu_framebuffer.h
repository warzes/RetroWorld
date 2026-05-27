#pragma once

#include "gpu_core.h"
#include "gpu_texture.h"

namespace gpu::fbo
{
	struct Framebuffer;
	using FramebufferPtr = std::shared_ptr<Framebuffer>;

	// Tells rhi what to do with a render target at the beginning of a pass
	enum class AttachmentLoadOp : uint32_t
	{
		// The previous contents of the image will be preserved
		Load,
		// The contents of the image will be cleared to a uniform value
		Clear,
		// The previous contents of the image need not be preserved (they may be discarded)
		DontCare,
	};

	struct RenderColorAttachment final
	{
		texture::TexturePtr texture;
		AttachmentLoadOp loadOp = AttachmentLoadOp::Load;
		float clearValue[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	struct ClearDepthStencilValue final
	{
		float depth{};
		int32_t stencil{};
	};

	struct RenderDepthStencilAttachment final
	{
		texture::TexturePtr texture;
		AttachmentLoadOp loadOp = AttachmentLoadOp::Load;
		ClearDepthStencilValue clearValue;
	};

	struct SwapchainRenderInfo final
	{
		Viewport viewport = {};
		AttachmentLoadOp colorLoadOp = AttachmentLoadOp::Load;
		float clearColorValue[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		AttachmentLoadOp depthLoadOp = AttachmentLoadOp::Load;
		float clearDepthValue = 0.0f;
		AttachmentLoadOp stencilLoadOp = AttachmentLoadOp::Load;
		int32_t clearStencilValue = 0;

		// If true, the linear->nonlinear sRGB OETF will be applied to pixels when rendering to the swapchain
		// This facility is provided because OpenGL does not expose the swapchain as an image we can interact with  in the usual manner.
		bool enableSrgb = true;
	};

	// Describes the framebuffer state when rendering with no attachments (e.g., for algorithms that output to images or buffers).
	// Consult the documentation for glFramebufferParameteri for more info.
	struct RenderNoAttachmentsInfo final
	{
		// An optional name to demarcate the pass in a graphics debugger
		std::string_view name;
		Viewport viewport{};
		core::Extent3D framebufferSize{}; // If depth > 0, framebuffer is layered
		SampleCount framebufferSamples{};
	};

	struct FramebufferCreateInfo final
	{
		std::optional<Viewport>                     viewport = std::nullopt;
		std::vector<RenderColorAttachment>          colorAttachments;
		std::optional<RenderDepthStencilAttachment> depthAttachment = std::nullopt;
		std::optional<RenderDepthStencilAttachment> stencilAttachment = std::nullopt;
	};

	FramebufferPtr CreateFramebuffer(const FramebufferCreateInfo& createInfo);

	[[nodiscard]] uint32_t Handle(FramebufferPtr fbo) noexcept;
	[[nodiscard]] bool IsValid(FramebufferPtr fbo) noexcept;
	[[nodiscard]] FramebufferCreateInfo GetCreateInfo(FramebufferPtr fbo) noexcept;

} // 