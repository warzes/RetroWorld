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