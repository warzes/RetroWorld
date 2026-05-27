#include "stdafx.h"
#include "gpu_buffer.h"
#include "core_log.h"
//=============================================================================
inline size_t roundUp(size_t numberToRoundUp, size_t multipleOf)
{
	assert(multipleOf);
	return ((numberToRoundUp + multipleOf - 1) / multipleOf) * multipleOf;
}
//=============================================================================
inline GLbitfield bufferStorageFlagsToGL(gpu::BufferStorageFlags flags) noexcept
{
	GLbitfield ret = 0;
	ret |= flags & gpu::BufferStorageFlag::DynamicStorage ? GL_DYNAMIC_STORAGE_BIT : 0;
	ret |= flags & gpu::BufferStorageFlag::ClientStorage ? GL_CLIENT_STORAGE_BIT : 0;

	// As far as I can tell, there is no perf hit to having both MAP_WRITE and MAP_READ all the time. Additionally, desktop platforms (the ones we care about) do not have incoherent host-visible device heaps, so we can safely include that flag all the time.
	// https://gpuopen.com/learn/get-the-most-out-of-smart-access-memory/
	// https://basnieuwenhuizen.nl/the-catastrophe-of-reading-from-vram/
	// https://asawicki.info/news_1740_vulkan_memory_types_on_pc_and_how_to_use_them
	constexpr GLenum memMapFlags = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
	ret |= flags & gpu::BufferStorageFlag::MapMemory ? memMapFlags : 0;
	return ret;
}
//=============================================================================
struct gpu::buffer::Buffer final
{
	Buffer() noexcept { glCreateBuffers(1, &id); }
	~Buffer()
	{
		if (id)
		{
			core::Debug("Destroyed buffer with handle " + std::to_string(id));
			if (mappedMemory) glUnmapNamedBuffer(id);
			glDeleteBuffers(1, &id);
		}
	}

	Buffer(const Buffer&) = delete;
	Buffer& operator=(const Buffer&) = delete;
	Buffer(Buffer&&) noexcept = default;
	Buffer& operator=(Buffer&&) noexcept = default;

	[[nodiscard]] operator bool() const noexcept { return id > 0; }
	[[nodiscard]] unsigned Handle() const noexcept { return id; }
	[[nodiscard]] bool IsValid() const noexcept { return id > 0; }

	uint32_t           id{ 0 };
	size_t             size{ 0 };
	BufferStorageFlags storageFlags{};
	void*              mappedMemory{ nullptr };
};
//=============================================================================
gpu::buffer::BufferPtr gpu::buffer::CreateBuffer(size_t size, BufferStorageFlags storageFlags, std::string_view name)
{
	return CreateBuffer(nullptr, size, storageFlags, name);
}
//=============================================================================
gpu::buffer::BufferPtr gpu::buffer::CreateBuffer(TriviallyCopyableByteSpan data, BufferStorageFlags storageFlags, std::string_view name)
{
	return CreateBuffer(data.data(), data.size_bytes(), storageFlags, name);
}
//=============================================================================
gpu::buffer::BufferPtr gpu::buffer::CreateBuffer(const void* data, size_t size, BufferStorageFlags storageFlags, std::string_view name)
{
	GLbitfield glflags = bufferStorageFlagsToGL(storageFlags);

	BufferPtr buffer = std::make_shared<Buffer>();
	buffer->size = roundUp(size, 16);
	buffer->storageFlags = storageFlags;
	glNamedBufferStorage(buffer->id, buffer->size, data, glflags);
	if (storageFlags & BufferStorageFlag::MapMemory)
	{
		// GL_MAP_UNSYNCHRONIZED_BIT should be used if the user can map and unmap buffers at their own will
		constexpr GLenum access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
		buffer->mappedMemory = glMapNamedBufferRange(buffer->id, 0, buffer->size, access);
	}

	if (!name.empty())
		glObjectLabel(GL_BUFFER, buffer->id, static_cast<GLsizei>(name.length()), name.data());

	core::Debug("Created buffer with handle " + std::to_string(buffer->id));
	return buffer;
}
//=============================================================================
void* gpu::buffer::GetMappedPointer(BufferPtr buffer) noexcept
{
	assert(buffer);
	return buffer->mappedMemory;
}
//=============================================================================
bool gpu::buffer::IsMapped(BufferPtr buffer) noexcept
{
	assert(buffer);
	return buffer->mappedMemory != nullptr;
}
//=============================================================================
size_t gpu::buffer::Size(BufferPtr buffer) noexcept
{
	assert(buffer);
	return buffer->size;
}
//=============================================================================
uint32_t gpu::buffer::Handle(BufferPtr buffer) noexcept
{
	return buffer ? buffer->Handle() : 0;
}
//=============================================================================
bool gpu::buffer::IsValid(BufferPtr buffer) noexcept
{
	return buffer ? buffer->IsValid() : false;
}
//=============================================================================
void gpu::buffer::Invalidate(BufferPtr buffer)
{
	assert(buffer);
	glInvalidateBufferData(buffer->id);
}
//=============================================================================
void gpu::buffer::FillData(BufferPtr buffer, const BufferFillInfo& clear)
{
	const auto actualSize = clear.size == WHOLE_BUFFER ? buffer->size : clear.size;
	assert(actualSize % 4 == 0 && "Size must be a multiple of 4 bytes");
	glClearNamedBufferSubData(buffer->id, GL_R32UI, clear.offset, actualSize, GL_RED_INTEGER, GL_UNSIGNED_INT, &clear.data);
}
//=============================================================================
void gpu::buffer::UpdateData(BufferPtr buffer, TriviallyCopyableByteSpan data, size_t destOffsetBytes)
{
	UpdateData(buffer, data.data(), data.size_bytes(), destOffsetBytes);
}
//=============================================================================
void gpu::buffer::UpdateData(BufferPtr buffer, const void* data, size_t size, size_t destOffsetBytes)
{
	assert(buffer);
	assert((buffer->storageFlags & BufferStorageFlag::DynamicStorage) && "UpdateData can only be called on buffers created with the DYNAMIC_STORAGE flag");
	assert(size + destOffsetBytes <= Size(buffer));
	glNamedBufferSubData(buffer->id, static_cast<GLuint>(destOffsetBytes), static_cast<GLuint>(size), data);
}
//=============================================================================