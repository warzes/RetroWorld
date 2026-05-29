#pragma once

#include "gpu_texture.h"
#include "gpu_buffer.h"

namespace gpu
{
	struct CopyTextureInfo final
	{
		texture::TexturePtr source;
		texture::TexturePtr target;
		uint32_t sourceLevel = 0;
		uint32_t targetLevel = 0;
		core::Offset3D sourceOffset = {};
		core::Offset3D targetOffset = {};
		core::Extent3D extent = {};
	};

	void CopyTexture(const CopyTextureInfo& copy);

	void TextureBarrier();

	struct CopyBufferInfo final
	{
		buffer::BufferPtr source;
		buffer::BufferPtr target;
		uint64_t sourceOffset = 0;
		uint64_t targetOffset = 0;
		// The amount of data to copy, in bytes. If size is WHOLE_BUFFER, the size of the source buffer is used.
		uint64_t size = buffer::WHOLE_BUFFER;
	};

	void CopyBuffer(const CopyBufferInfo& copy);

	struct CopyTextureToBufferInfo final
	{
		texture::TexturePtr sourceTexture;
		buffer::BufferPtr targetBuffer;
		uint32_t level = 0;
		core::Offset3D sourceOffset = {};
		uint64_t targetOffset = {};
		core::Extent3D extent = {};
		UploadFormat format = UploadFormat::INFER_FORMAT;
		UploadType type = UploadType::INFER_TYPE;

		// Specifies, in texels, the size of rows in the buffer (for 2D and 3D images). If zero, it is assumed to be tightly packed according to \p extent
		uint32_t bufferRowLength = 0;

		// Specifies, in texels, the number of rows in the buffer (for 3D images. If zero, it is assumed to be tightly packed according to \p extent
		uint32_t bufferImageHeight = 0;
	};

	void CopyTextureToBuffer(const CopyTextureToBufferInfo& copy);

	struct CopyBufferToTextureInfo final
	{
		buffer::BufferPtr sourceBuffer;
		texture::TexturePtr targetTexture;
		uint32_t level = 0;
		uint64_t sourceOffset = {};
		core::Offset3D targetOffset = {};
		core::Extent3D extent = {};

		// The arrangement of components of texels in the source buffer. DEPTH_STENCIL is not allowed here
		UploadFormat format = UploadFormat::INFER_FORMAT;

		// The data type of the texel data
		UploadType type = UploadType::INFER_TYPE;

		// Specifies, in texels, the size of rows in the buffer (for 2D and 3D images). If zero, it is assumed to be tightly packed according to \p extent
		uint32_t bufferRowLength = 0;

		// Specifies, in texels, the number of rows in the buffer (for 3D images. If zero, it is assumed to be tightly packed according to \p extent
		uint32_t bufferImageHeight = 0;
	};

	void CopyBufferToTexture(const CopyBufferToTextureInfo& copy);

	void MemoryBarrier(MemoryBarrierBits accessBits);

} //namespace gpu