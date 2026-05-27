#include "stdafx.h"
#include "_gpu_contextState.h"
//=============================================================================
bool gpu::detail::RenderAttachments::operator==(const RenderAttachments& rhs) const
{
	if (colorAttachments.size() != rhs.colorAttachments.size())
		return false;

	// Crucially, two attachments with the same address are not necessarily the same.
	// The inverse is also true: two attachments with different addresses are not necessarily different.

	for (size_t i = 0; i < colorAttachments.size(); i++)
	{
		// Color attachments must be non-null
		if (colorAttachments[i] != rhs.colorAttachments[i])
			return false;
	}

	// Nullity of the attachments differ
	if ((depthAttachment && !rhs.depthAttachment) || (!depthAttachment && rhs.depthAttachment))
		return false;
	// Both attachments are non-null, but have different values
	if (depthAttachment && rhs.depthAttachment && (*depthAttachment != *rhs.depthAttachment))
		return false;

	if ((stencilAttachment && !rhs.stencilAttachment) || (!stencilAttachment && rhs.stencilAttachment))
		return false;
	if (stencilAttachment && rhs.stencilAttachment && (*stencilAttachment != *rhs.stencilAttachment))
		return false;

	return true;
}
//=============================================================================
void gpu::ContextState::BeginFrame()
{
	isRendering = true;
}
//=============================================================================
void gpu::ContextState::EndFrame()
{
	currentVertexArray = nullptr;
	currentShaderProgram = nullptr;

	isContextResize = false;
	isRendering = false;
	isRenderingToSwapchain = true;
	isIndexBufferBound = false;

	if (scissorEnabled)
	{
		glDisable(GL_SCISSOR_TEST);
		scissorEnabled = false;
	}
}
//=============================================================================
void gpu::ContextState::Clear()
{
	vertexArrayCache.clear();
	samplerCache.clear();
	framebufferCacheKey.clear();
	framebufferCacheValue.clear();

	*this = {};
}
//=============================================================================