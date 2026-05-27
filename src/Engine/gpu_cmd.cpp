#include "stdafx.h"
#include "gpu_cmd.h"
#include "_gpu_contextState.h"
#include "_gpu_enumDesc.h"
//=============================================================================
inline void setViewportInternal(const gpu::Viewport& viewport, const gpu::Viewport& lastViewport, bool initViewport)
{
	if (initViewport || viewport.drawRect != lastViewport.drawRect)
	{
		glViewport(viewport.drawRect.offset.x,
			viewport.drawRect.offset.y,
			viewport.drawRect.extent.width,
			viewport.drawRect.extent.height);
	}
	if (initViewport || viewport.minDepth != lastViewport.minDepth || viewport.maxDepth != lastViewport.maxDepth)
	{
		glDepthRangef(viewport.minDepth, viewport.maxDepth);
	}
	if (initViewport || viewport.depthRange != lastViewport.depthRange)
	{
		glClipControl(GL_LOWER_LEFT, gpu::EnumToValue(viewport.depthRange));
	}
}
//=============================================================================
inline gpu::fbo::FramebufferPtr makeSingleTextureFbo(gpu::texture::TexturePtr texture)
{
	auto format = gpu::texture::GetCreateInfo(texture).format;

	auto depthStencil = gpu::fbo::RenderDepthStencilAttachment{ .texture = texture };
	auto color = gpu::fbo::RenderColorAttachment{ .texture = texture };
	gpu::fbo::FramebufferCreateInfo createInfo;

	if (gpu::IsDepthFormat(format))
		createInfo.depthAttachment = depthStencil;

	if (gpu::IsStencilFormat(format))
		createInfo.stencilAttachment = depthStencil;

	if (gpu::IsColorFormat(format))
		createInfo.colorAttachments.push_back(color);

	return gpu::fbo::CreateFramebuffer(createInfo);
}
//=============================================================================
void gpu::cmd::BlitTexture(texture::TexturePtr source, texture::TexturePtr target, core::Offset3D sourceOffset, core::Offset3D targetOffset, core::Extent3D sourceExtent, core::Extent3D targetExtent, Filter filter, AspectMask aspect)
{
	auto fboSource = makeSingleTextureFbo(source);
	auto fboTarget = makeSingleTextureFbo(target);
	glBlitNamedFramebuffer(
		gpu::fbo::Handle(fboSource),
		gpu::fbo::Handle(fboTarget),
		sourceOffset.x,
		sourceOffset.y,
		sourceExtent.width,
		sourceExtent.height,
		targetOffset.x,
		targetOffset.y,
		targetExtent.width,
		targetExtent.height,
		gpu::EnumToValue(aspect),
		gpu::EnumToValue(filter));
}
//=============================================================================
void gpu::cmd::BlitTextureToSwapchain(texture::TexturePtr source, core::Offset3D sourceOffset, core::Offset3D targetOffset, core::Extent3D sourceExtent, core::Extent3D targetExtent, Filter filter, AspectMask aspect)
{
	auto fbo = makeSingleTextureFbo(source);

	glBlitNamedFramebuffer(gpu::fbo::Handle(fbo),
		0,
		sourceOffset.x,
		sourceOffset.y,
		sourceExtent.width,
		sourceExtent.height,
		targetOffset.x,
		targetOffset.y,
		targetExtent.width,
		targetExtent.height,
		gpu::EnumToValue(aspect),
		gpu::EnumToValue(filter));
}
//=============================================================================
void gpu::cmd::SwapchainRendering(const fbo::SwapchainRenderInfo& renderInfo)
{
	const auto& ri = renderInfo;

	if (!context.isRenderingToSwapchain) 
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	context.isRenderingToSwapchain = true;

	switch (ri.colorLoadOp)
	{
	case fbo::AttachmentLoadOp::Load: break;
	case fbo::AttachmentLoadOp::Clear:
	{
		if (context.lastColorMask[0] != ColorComponentFlag::RGBA_BITS)
		{
			glColorMaski(0, true, true, true, true);
			context.lastColorMask[0] = ColorComponentFlag::RGBA_BITS;
		}
		glClearNamedFramebufferfv(0, GL_COLOR, 0, ri.clearColorValue);
		break;
	}
	case fbo::AttachmentLoadOp::DontCare:
	{
		GLenum attachment = GL_COLOR;
		glInvalidateNamedFramebufferData(0, 1, &attachment);
		break;
	}
	default: std::unreachable();
	}

	switch (ri.depthLoadOp)
	{
	case fbo::AttachmentLoadOp::Load: break;
	case fbo::AttachmentLoadOp::Clear:
	{
		if (context.lastDepthMask == false)
		{
			glDepthMask(true);
			context.lastDepthMask = true;
		}
		glClearNamedFramebufferfv(0, GL_DEPTH, 0, &ri.clearDepthValue);
		break;
	}
	case fbo::AttachmentLoadOp::DontCare:
	{
		GLenum attachment = GL_DEPTH;
		glInvalidateNamedFramebufferData(0, 1, &attachment);
		break;
	}
	default: std::unreachable();
	}

	switch (ri.stencilLoadOp)
	{
	case fbo::AttachmentLoadOp::Load: break;
	case fbo::AttachmentLoadOp::Clear:
	{
		if (context.lastStencilMask[0] == false || context.lastStencilMask[1] == false)
		{
			glStencilMask(true);
			context.lastStencilMask[0] = true;
			context.lastStencilMask[1] = true;
		}
		glClearNamedFramebufferiv(0, GL_STENCIL, 0, &ri.clearStencilValue);
		break;
	}
	case fbo::AttachmentLoadOp::DontCare:
	{
		GLenum attachment = GL_STENCIL;
		glInvalidateNamedFramebufferData(0, 1, &attachment);
		break;
	}
	default: std::unreachable();
	}

	setViewportInternal(renderInfo.viewport, context.lastViewport, context.initViewport);

	context.lastViewport = renderInfo.viewport;
	context.initViewport = false;
}
//=============================================================================
void gpu::cmd::BindFramebuffer(fbo::FramebufferPtr fbo)
{
	context.isRenderingToSwapchain = false;

	GLuint fboId = fbo::Handle(fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fboId);

	const auto& ri = fbo::GetCreateInfo(fbo);

	for (GLint i = 0; i < static_cast<GLint>(ri.colorAttachments.size()); i++)
	{
		const auto& attachment = ri.colorAttachments[i];
		switch (attachment.loadOp)
		{
		case fbo::AttachmentLoadOp::Load: break;
		case fbo::AttachmentLoadOp::Clear:
		{
			if (context.lastColorMask[i] != ColorComponentFlag::RGBA_BITS)
			{
				glColorMaski(i, true, true, true, true);
				context.lastColorMask[i] = ColorComponentFlag::RGBA_BITS;
			}

			glClearNamedFramebufferfv(fboId, GL_COLOR, i, attachment.clearValue);
			break;
		}
		case fbo::AttachmentLoadOp::DontCare:
		{
			GLenum colorAttachment = GL_COLOR_ATTACHMENT0 + i;
			glInvalidateNamedFramebufferData(fboId, 1, &colorAttachment);
			break;
		}
		default: std::unreachable();
		}
	}

	if (ri.depthAttachment)
	{
		switch (ri.depthAttachment->loadOp)
		{
		case fbo::AttachmentLoadOp::Load: break;
		case fbo::AttachmentLoadOp::Clear:
		{
			// clear just depth
			if (context.lastDepthMask == false)
			{
				glDepthMask(true);
				context.lastDepthMask = true;
			}

			glClearNamedFramebufferfv(fboId, GL_DEPTH, 0, &ri.depthAttachment->clearValue.depth);
			break;
		}
		case fbo::AttachmentLoadOp::DontCare:
		{
			GLenum attachment = GL_DEPTH_ATTACHMENT;
			glInvalidateNamedFramebufferData(fboId, 1, &attachment);
			break;
		}
		default: std::unreachable();
		}
	}

	if (ri.stencilAttachment)
	{
		switch (ri.stencilAttachment->loadOp)
		{
		case fbo::AttachmentLoadOp::Load: break;
		case fbo::AttachmentLoadOp::Clear:
		{
			// clear just stencil
			if (context.lastStencilMask[0] == false || context.lastStencilMask[1] == false)
			{
				glStencilMask(true);
				context.lastStencilMask[0] = true;
				context.lastStencilMask[1] = true;
			}

			glClearNamedFramebufferiv(fboId, GL_STENCIL, 0, &ri.stencilAttachment->clearValue.stencil);
			break;
		}
		case fbo::AttachmentLoadOp::DontCare:
		{
			GLenum attachment = GL_STENCIL_ATTACHMENT;
			glInvalidateNamedFramebufferData(fboId, 1, &attachment);
			break;
		}
		default: std::unreachable();
		}
	}

	Viewport viewport{};
	if (ri.viewport)
	{
		viewport = *ri.viewport;
	}
	else
	{
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// determine intersection of all render targets
		core::Rect2D drawRect{
		.offset = {},
		.extent = {std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max()},
		};
		for (const auto& attachment : ri.colorAttachments)
		{
			drawRect.extent.width = std::min(drawRect.extent.width, texture::GetCreateInfo(attachment.texture).extent.width);
			drawRect.extent.height =
				std::min(drawRect.extent.height, texture::GetCreateInfo(attachment.texture).extent.height);
		}
		if (ri.depthAttachment)
		{
			drawRect.extent.width =
				std::min(drawRect.extent.width, texture::GetCreateInfo(ri.depthAttachment->texture).extent.width);
			drawRect.extent.height =
				std::min(drawRect.extent.height, texture::GetCreateInfo(ri.depthAttachment->texture).extent.height);
		}
		if (ri.stencilAttachment)
		{
			drawRect.extent.width =
				std::min(drawRect.extent.width, texture::GetCreateInfo(ri.stencilAttachment->texture).extent.width);
			drawRect.extent.height =
				std::min(drawRect.extent.height, texture::GetCreateInfo(ri.stencilAttachment->texture).extent.height);
		}
		viewport.drawRect = drawRect;
	}

	setViewportInternal(viewport, context.lastViewport, context.initViewport);

	context.lastViewport = viewport;
	context.initViewport = false;
}
//=============================================================================
void gpu::cmd::BindFramebufferNoAttachments(fbo::FramebufferPtr fbo, const fbo::RenderNoAttachmentsInfo& info)
{
	BindFramebuffer(fbo);
	GLuint fboId = fbo::Handle(fbo);

	glNamedFramebufferParameteri(fboId, GL_FRAMEBUFFER_DEFAULT_WIDTH, info.framebufferSize.width);
	glNamedFramebufferParameteri(fboId, GL_FRAMEBUFFER_DEFAULT_HEIGHT, info.framebufferSize.height);
	glNamedFramebufferParameteri(fboId, GL_FRAMEBUFFER_DEFAULT_LAYERS, info.framebufferSize.depth);
	glNamedFramebufferParameteri(fboId, GL_FRAMEBUFFER_DEFAULT_SAMPLES, EnumToValue(info.framebufferSamples));
	glNamedFramebufferParameteri(fboId, GL_FRAMEBUFFER_DEFAULT_FIXED_SAMPLE_LOCATIONS, GL_TRUE);
}
//=============================================================================
void gpu::cmd::SetTopology(PrimitiveTopology topology)
{
	context.currentTopology = topology;
}
//=============================================================================
void gpu::cmd::SetViewport(const Viewport& viewport)
{
	assert(context.isRendering);
	setViewportInternal(viewport, context.lastViewport, false);
	context.lastViewport = viewport;
}
//=============================================================================
void gpu::cmd::SetScissor(const Scissor& scissor)
{
	assert(context.isRendering);

	if (!context.scissorEnabled)
	{
		glEnable(GL_SCISSOR_TEST);
		context.scissorEnabled = true;
	}

	if (scissor == context.lastScissor) return;

	glScissor(scissor.position.x, scissor.position.y, scissor.size.x, scissor.size.y);

	context.lastScissor = scissor;
}
//=============================================================================
void gpu::cmd::BindSampledImage(uint32_t index, texture::TexturePtr texture, texture::SamplerPtr sampler)
{
	assert(gpu::context.isRendering);
	assert(texture);
	assert(sampler);

	glBindTextureUnit(index, texture::Handle(texture));
	glBindSampler(index, texture::Handle(sampler));
}
//=============================================================================
void gpu::cmd::BindImage(uint32_t index, texture::TexturePtr texture, uint32_t level)
{
	assert(context.isRendering);
	assert(level < texture::GetCreateInfo(texture).mipLevels);
	assert(IsValidImageFormat(texture::GetCreateInfo(texture).format));

	glBindImageTexture(index,
		texture::Handle(texture),
		level,
		GL_TRUE,
		0,
		GL_READ_WRITE,
		EnumToValue(texture::GetCreateInfo(texture).format));
}
//=============================================================================
void gpu::cmd::BindVertexBuffer(vao::VertexArrayPtr vao, uint32_t bindingIndex, gpu::buffer::BufferPtr buffer, uint64_t offset, uint64_t stride)
{
	assert(context.isRendering);
	glVertexArrayVertexBuffer(vao::Handle(vao),
		bindingIndex,
		buffer::Handle(buffer),
		static_cast<GLintptr>(offset),
		static_cast<GLsizei>(stride));
}
//=============================================================================
void gpu::cmd::BindIndexBuffer(vao::VertexArrayPtr vao, gpu::buffer::BufferPtr buffer, IndexType indexType)
{
	assert(context.isRendering);

	context.isIndexBufferBound = true;
	context.currentIndexType = indexType;
	glVertexArrayElementBuffer(vao::Handle(vao), gpu::buffer::Handle(buffer));
}
//=============================================================================
void gpu::cmd::BindUniformBuffer(uint32_t index, buffer::BufferPtr buffer, uint64_t offset, uint64_t size)
{
	assert(context.isRendering);
	if (size == WHOLE_BUFFER) size = buffer::Size(buffer) - offset;
	glBindBufferRange(GL_UNIFORM_BUFFER, index, buffer::Handle(buffer), offset, size);
}
//=============================================================================
void gpu::cmd::BindStorageBuffer(uint32_t index, buffer::BufferPtr buffer, uint64_t offset, uint64_t size)
{
	assert(context.isRendering);
	if (size == WHOLE_BUFFER) size = buffer::Size(buffer) - offset;
	glBindBufferRange(GL_SHADER_STORAGE_BUFFER, index, buffer::Handle(buffer), offset, size);
}
//=============================================================================
void gpu::cmd::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
	assert(context.isRendering);
	glDrawArraysInstancedBaseInstance(EnumToValue(context.currentTopology),
		firstVertex,
		vertexCount,
		instanceCount,
		firstInstance);
}
//=============================================================================
void gpu::cmd::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
	assert(context.isRendering);
	assert(context.isIndexBufferBound);

	// double cast is needed to prevent compiler from complaining about 32->64 bit pointer cast
	glDrawElementsInstancedBaseVertexBaseInstance(
		EnumToValue(context.currentTopology),
		indexCount,
		EnumToValue(context.currentIndexType),
		reinterpret_cast<void*>(static_cast<uintptr_t>(firstIndex * GetIndexSize(context.currentIndexType))),
		instanceCount,
		vertexOffset,
		firstInstance);
}
//=============================================================================
void gpu::cmd::DrawIndirect(buffer::BufferPtr commandBuffer, uint64_t commandBufferOffset, uint32_t drawCount, uint32_t stride)
{
	assert(context.isRendering);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer::Handle(commandBuffer));
	glMultiDrawArraysIndirect(EnumToValue(context.currentTopology),
		reinterpret_cast<void*>(static_cast<uintptr_t>(commandBufferOffset)),
		drawCount,
		stride);
}
//=============================================================================
void gpu::cmd::DrawIndirectCount(buffer::BufferPtr commandBuffer, uint64_t commandBufferOffset, buffer::BufferPtr countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	assert(context.isRendering);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer::Handle(commandBuffer));
	glBindBuffer(GL_PARAMETER_BUFFER, buffer::Handle(countBuffer));
	glMultiDrawArraysIndirectCount(EnumToValue(context.currentTopology),
		reinterpret_cast<void*>(static_cast<uintptr_t>(commandBufferOffset)),
		static_cast<GLintptr>(countBufferOffset),
		maxDrawCount,
		stride);

}
//=============================================================================
void gpu::cmd::DrawIndexedIndirect(buffer::BufferPtr commandBuffer, uint64_t commandBufferOffset, uint32_t drawCount, uint32_t stride)
{
	assert(context.isRendering);
	assert(context.isIndexBufferBound);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer::Handle(commandBuffer));
	glMultiDrawElementsIndirect(EnumToValue(context.currentTopology),
		EnumToValue(context.currentIndexType),
		reinterpret_cast<void*>(static_cast<uintptr_t>(commandBufferOffset)),
		drawCount,
		stride);

}
//=============================================================================
void gpu::cmd::DrawIndexedIndirectCount(buffer::BufferPtr commandBuffer, uint64_t commandBufferOffset, buffer::BufferPtr countBuffer, uint64_t countBufferOffset, uint32_t maxDrawCount, uint32_t stride)
{
	assert(context.isRendering);
	assert(context.isIndexBufferBound);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, buffer::Handle(commandBuffer));
	glBindBuffer(GL_PARAMETER_BUFFER, buffer::Handle(countBuffer));
	glMultiDrawElementsIndirectCount(EnumToValue(context.currentTopology),
		EnumToValue(context.currentIndexType),
		reinterpret_cast<void*>(static_cast<uintptr_t>(commandBufferOffset)),
		static_cast<GLintptr>(countBufferOffset),
		maxDrawCount,
		stride);
}
//=============================================================================
void gpu::cmd::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
{
	glDispatchCompute(groupCountX, groupCountY, groupCountZ);
}
//=============================================================================
void gpu::cmd::DispatchIndirect(buffer::BufferPtr commandBuffer, uint64_t commandBufferOffset)
{
	glBindBuffer(GL_DISPATCH_INDIRECT_BUFFER, buffer::Handle(commandBuffer));
	glDispatchComputeIndirect(static_cast<GLintptr>(commandBufferOffset));
}
//=============================================================================