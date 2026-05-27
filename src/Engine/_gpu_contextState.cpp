#include "stdafx.h"
#include "_gpu_contextState.h"
#include "_gpu_enumDesc.h"
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
void gpu::ContextState::Init(uint16_t width, uint16_t height)
{
	context.contextWidth = width;
	context.contextHeight = height;

	void GLEnableOrDisable(GLenum state, GLboolean value) noexcept;

	GLEnableOrDisable(GL_PRIMITIVE_RESTART_FIXED_INDEX, inputAssemblyState.primitiveRestartEnable);
	
	GLEnableOrDisable(GL_DEPTH_CLAMP, rasterizationState.depthClampEnable);
	glPolygonMode(GL_FRONT_AND_BACK, EnumToValue(rasterizationState.polygonMode));
	GLEnableOrDisable(GL_CULL_FACE, rasterizationState.cullMode != CullFace::None);
	glCullFace(EnumToValue(rasterizationState.cullMode));
	glFrontFace(EnumToValue(rasterizationState.frontFace));
	GLEnableOrDisable(GL_POLYGON_OFFSET_FILL, rasterizationState.depthBiasEnable);
	GLEnableOrDisable(GL_POLYGON_OFFSET_LINE, rasterizationState.depthBiasEnable);
	GLEnableOrDisable(GL_POLYGON_OFFSET_POINT, rasterizationState.depthBiasEnable);
	glPolygonOffset(rasterizationState.depthBiasSlopeFactor, rasterizationState.depthBiasConstantFactor);
	glLineWidth(rasterizationState.lineWidth);
	glPointSize(rasterizationState.pointSize);

	GLEnableOrDisable(GL_SAMPLE_SHADING, multisampleState.sampleShadingEnable);
	glMinSampleShading(multisampleState.minSampleShading);
	GLEnableOrDisable(GL_SAMPLE_MASK, multisampleState.sampleMask != 0xFFFFFFFF);
	glSampleMaski(0, multisampleState.sampleMask);
	GLEnableOrDisable(GL_SAMPLE_ALPHA_TO_COVERAGE, multisampleState.alphaToCoverageEnable);
	GLEnableOrDisable(GL_SAMPLE_ALPHA_TO_ONE, multisampleState.alphaToOneEnable);

	GLEnableOrDisable(GL_DEPTH_TEST, depthState.depthTestEnable);
	glDepthMask(depthState.depthWriteEnable);
	glDepthFunc(EnumToValue(depthState.depthCompareOp));

	GLEnableOrDisable(GL_STENCIL_TEST, stencilState.stencilTestEnable);
	
	GLEnableOrDisable(GL_COLOR_LOGIC_OP, colorBlendState.logicOpEnable);
	glBlendColor(colorBlendState.blendConstants[0], colorBlendState.blendConstants[1], colorBlendState.blendConstants[2], colorBlendState.blendConstants[3]);
	GLEnableOrDisable(GL_BLEND, GL_FALSE);
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