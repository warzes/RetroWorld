#pragma once

#include "gpu_program.h"
#include "gpu_texture.h"
#include "gpu_vao.h"
#include "gpu_buffer.h"
#include "gpu_framebuffer.h"
#include "gpu_pipeline.h"

namespace gpu::cmd
{
	void BlitTexture(
		texture::TexturePtr source,
		texture::TexturePtr target,
		core::Offset3D sourceOffset,
		core::Offset3D targetOffset,
		core::Extent3D sourceExtent,
		core::Extent3D targetExtent,
		Filter filter,
		AspectMask aspect = AspectMaskBit::COLOR_BUFFER_BIT);

	void BlitTextureToSwapchain(texture::TexturePtr source,
		core::Offset3D sourceOffset,
		core::Offset3D targetOffset,
		core::Extent3D sourceExtent,
		core::Extent3D targetExtent,
		Filter filter,
		AspectMask aspect = AspectMaskBit::COLOR_BUFFER_BIT);

	void SwapchainRendering(const fbo::SwapchainRenderInfo& renderInfo);
	void BindFramebuffer(fbo::FramebufferPtr fbo);
	void BindFramebufferNoAttachments(fbo::FramebufferPtr fbo, const fbo::RenderNoAttachmentsInfo& info);

	void SetTopology(PrimitiveTopology topology);
	
	void SetState(const InputAssemblyState& state);
	void SetState(const TessellationState& state);
	void SetState(const RasterizationState& state);
	void SetState(const MultisampleState& state);
	void SetState(const DepthState& state);
	void SetState(const StencilState& state);
	void SetState(const ColorBlendState& state);

	void SetViewport(const Viewport& viewport);
	void SetScissor(const Scissor& scissor);

	inline void BindShaderProgram(program::ShaderProgramPtr program)
	{
		program::BindShaderProgram(program);
	}
	inline void BindVertexArray(vao::VertexArrayPtr vao)
	{
		vao::BindVertexArray(vao);
	}
	
	void BindSampledImage(uint32_t index, texture::TexturePtr texture, texture::SamplerPtr sampler);

	void BindImage(uint32_t index, texture::TexturePtr texture, uint32_t level);

	void BindVertexBuffer(vao::VertexArrayPtr vao, uint32_t bindingIndex, buffer::BufferPtr buffer, uint64_t offset, uint64_t stride);
	void BindIndexBuffer(vao::VertexArrayPtr vao, buffer::BufferPtr buffer, IndexType indexType);

	void BindUniformBuffer(uint32_t index, buffer::BufferPtr buffer, uint64_t offset, uint64_t size);
	void BindStorageBuffer(uint32_t index, buffer::BufferPtr buffer, uint64_t offset = 0, uint64_t size = WHOLE_BUFFER);

	// Equivalent to glDrawArraysInstancedBaseInstance or vkCmdDraw
	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
	// Equivalent to glDrawElementsInstancedBaseVertexBaseInstance or vkCmdDrawIndexed
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance);

	// Equivalent to glMultiDrawArraysIndirect or vkCmdDrawDrawIndirect
	void DrawIndirect(buffer::BufferPtr commandBuffer,
		uint64_t commandBufferOffset,
		uint32_t drawCount,
		uint32_t stride);

	// Equivalent to glMultiDrawArraysIndirectCount or vkCmdDrawIndirectCount
	void DrawIndirectCount(buffer::BufferPtr commandBuffer,
		uint64_t commandBufferOffset,
		buffer::BufferPtr countBuffer,
		uint64_t countBufferOffset,
		uint32_t maxDrawCount,
		uint32_t stride);

	// Equivalent to glMultiDrawElementsIndirect or vkCmdDrawIndexedIndirect
	void DrawIndexedIndirect(buffer::BufferPtr commandBuffer,
		uint64_t commandBufferOffset,
		uint32_t drawCount,
		uint32_t stride);

	// Equivalent to glMultiDrawElementsIndirectCount or vkCmdDrawIndexedIndirectCount
	void DrawIndexedIndirectCount(buffer::BufferPtr commandBuffer,
		uint64_t commandBufferOffset,
		buffer::BufferPtr countBuffer,
		uint64_t countBufferOffset,
		uint32_t maxDrawCount,
		uint32_t stride);

	void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ);
	void DispatchIndirect(buffer::BufferPtr commandBuffer, uint64_t commandBufferOffset);

} // namespace gpu::cmd