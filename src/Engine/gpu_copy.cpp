#include "stdafx.h"
#include "gpu_copy.h"
#include "_gpu_contextState.h"
#include "_gpu_enumDesc.h"
//=============================================================================
void gpu::CopyTexture(const CopyTextureInfo& copy)
{
	glCopyImageSubData(texture::Handle(copy.source),
		gpu::EnumToValue(texture::GetCreateInfo(copy.source).imageType),
		copy.sourceLevel,
		copy.sourceOffset.x,
		copy.sourceOffset.y,
		copy.sourceOffset.z,
		texture::Handle(copy.target),
		gpu::EnumToValue(texture::GetCreateInfo(copy.target).imageType),
		copy.targetLevel,
		copy.targetOffset.x,
		copy.targetOffset.y,
		copy.targetOffset.z,
		copy.extent.width,
		copy.extent.height,
		copy.extent.depth);
}
//=============================================================================
void gpu::TextureBarrier()
{
	glTextureBarrier();
}
//=============================================================================
void gpu::CopyBuffer(const CopyBufferInfo& copy)
{
	auto size = copy.size;
	if (size == WHOLE_BUFFER)
	{
		size = buffer::Size(copy.source) - copy.sourceOffset;
	}

	glCopyNamedBufferSubData(buffer::Handle(copy.source),
		buffer::Handle(copy.target),
		static_cast<GLintptr>(copy.sourceOffset),
		static_cast<GLintptr>(copy.targetOffset),
		static_cast<GLsizeiptr>(size));
}
//=============================================================================
void gpu::CopyTextureToBuffer(const CopyTextureToBufferInfo& copy)
{
	glPixelStorei(GL_PACK_ROW_LENGTH, copy.bufferRowLength);
	glPixelStorei(GL_PACK_IMAGE_HEIGHT, copy.bufferImageHeight);

	glBindBuffer(GL_PIXEL_PACK_BUFFER, buffer::Handle(copy.targetBuffer));

	GLenum format{};
	if (copy.format == UploadFormat::INFER_FORMAT)
	{
		format = EnumToValue(FormatToUploadFormat(texture::GetCreateInfo(copy.sourceTexture).format));
	}
	else
	{
		format = EnumToValue(copy.format);
	}

	GLenum type{};
	if (copy.type == UploadType::INFER_TYPE)
	{
		type = EnumToValue(texture::GetCreateInfo(copy.sourceTexture).format);
	}
	else
	{
		type = EnumToValue(copy.type);
	}

	glGetTextureSubImage(texture::Handle(copy.sourceTexture),
		copy.level,
		copy.sourceOffset.x,
		copy.sourceOffset.z,
		copy.sourceOffset.z,
		copy.extent.width,
		copy.extent.height,
		copy.extent.depth,
		format,
		type,
		static_cast<GLsizei>(buffer::Size(copy.targetBuffer)),
		reinterpret_cast<void*>(static_cast<uintptr_t>(copy.targetOffset)));
}
//=============================================================================
void gpu::CopyBufferToTexture(const CopyBufferToTextureInfo& copy)
{
	glPixelStorei(GL_UNPACK_ROW_LENGTH, copy.bufferRowLength);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, copy.bufferImageHeight);

	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, buffer::Handle(copy.sourceBuffer));

	void SubImageInternal(texture::TexturePtr texture, const texture::TextureUpdateInfo& info);

	SubImageInternal(copy.targetTexture, { copy.level,
		copy.targetOffset,
		copy.extent,
		copy.format,
		copy.type,
		reinterpret_cast<void*>(static_cast<uintptr_t>(copy.sourceOffset)),
		copy.bufferRowLength,
		copy.bufferImageHeight });
}
//=============================================================================
void gpu::MemoryBarrier(MemoryBarrierBits accessBits)
{
	glMemoryBarrier(EnumToValue(accessBits));
}
//=============================================================================