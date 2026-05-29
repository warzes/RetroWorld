#include "stdafx.h"
#include "gpu_framebuffer.h"
#include "_gpu_contextState.h"
#include "core_log.h"
//=============================================================================
struct gpu::fbo::Framebuffer final
{
	Framebuffer() noexcept { glCreateFramebuffers(1, &id); }
	~Framebuffer()
	{
		if (id)
		{
			core::Debug("Destroyed framebuffer with handle" + std::to_string(id));
			glDeleteFramebuffers(1, &id);
		}
	}

	Framebuffer(const Framebuffer&) noexcept = default;
	Framebuffer& operator=(const Framebuffer&) noexcept = default;
	Framebuffer(Framebuffer&&) noexcept = default;
	Framebuffer& operator=(Framebuffer&&) noexcept = default;

	[[nodiscard]] operator bool() const noexcept { return id > 0; }
	[[nodiscard]] uint32_t Handle() const noexcept { return id; }
	[[nodiscard]] bool IsValid() const noexcept { return id > 0; }

	uint32_t              id{ 0 };
	FramebufferCreateInfo createInfo;
};
//=============================================================================
gpu::fbo::FramebufferPtr gpu::fbo::CreateFramebuffer(const FramebufferCreateInfo& createInfo)
{
	detail::RenderAttachments attachments;
	for (const auto& colorAttachment : createInfo.colorAttachments)
	{
		attachments.colorAttachments.emplace_back(detail::TextureProxy{
			texture::GetCreateInfo(colorAttachment.texture),
			texture::Handle(colorAttachment.texture),
			});
	}
	if (createInfo.depthAttachment)
	{
		attachments.depthAttachment.emplace(detail::TextureProxy{
			texture::GetCreateInfo(createInfo.depthAttachment->texture),
			texture::Handle(createInfo.depthAttachment->texture),
			});
	}
	if (createInfo.stencilAttachment)
	{
		attachments.stencilAttachment.emplace(detail::TextureProxy{
			texture::GetCreateInfo(createInfo.stencilAttachment->texture),
			texture::Handle(createInfo.stencilAttachment->texture),
			});
	}

	for (size_t i = 0; i < context.framebufferCacheKey.size(); i++)
	{
		if (context.framebufferCacheKey[i] == attachments)
			return context.framebufferCacheValue[i];
	}

	auto fbo = std::make_shared<Framebuffer>();
	fbo->createInfo = createInfo;

	std::vector<GLenum> drawBuffers;
	for (size_t i = 0; i < attachments.colorAttachments.size(); i++)
	{
		const auto& attachment = attachments.colorAttachments[i];
		glNamedFramebufferTexture(fbo->id, static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i), attachment.id, 0);
		drawBuffers.push_back(static_cast<GLenum>(GL_COLOR_ATTACHMENT0 + i));
	}
	glNamedFramebufferDrawBuffers(fbo->id, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

	if (attachments.depthAttachment && attachments.stencilAttachment &&
		attachments.depthAttachment == attachments.stencilAttachment)
	{
		glNamedFramebufferTexture(fbo->id, GL_DEPTH_STENCIL_ATTACHMENT, attachments.depthAttachment->id, 0);
	}
	else
	{
		if (attachments.depthAttachment)
		{
			glNamedFramebufferTexture(fbo->id, GL_DEPTH_ATTACHMENT, attachments.depthAttachment->id, 0);
		}

		if (attachments.stencilAttachment)
		{
			glNamedFramebufferTexture(fbo->id, GL_STENCIL_ATTACHMENT, attachments.stencilAttachment->id, 0);
		}
	}

	core::Debug("Created framebuffer with handle " + std::to_string(fbo->id));

	context.framebufferCacheKey.emplace_back(std::move(attachments));
	return context.framebufferCacheValue.emplace_back(fbo);
}
//=============================================================================
uint32_t gpu::fbo::Handle(const FramebufferPtr& fbo) noexcept
{
	return fbo ? fbo->Handle() : 0;
}
//=============================================================================
bool gpu::fbo::IsValid(const FramebufferPtr& fbo) noexcept
{
	return fbo ? fbo->IsValid() : false;
}
//=============================================================================
gpu::fbo::FramebufferCreateInfo gpu::fbo::GetCreateInfo(const FramebufferPtr& fbo) noexcept
{
	return fbo ? fbo->createInfo : FramebufferCreateInfo{};
}
//=============================================================================