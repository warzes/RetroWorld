#pragma once

#include "gpu_core.h"
#include "core_flags.h"

namespace gpu::buffer
{
	struct Buffer;
	using BufferPtr = std::shared_ptr<Buffer>;

	enum class BufferStorageFlag : uint32_t
	{
		None = 0,
		// Allows the user to update the buffer's contents with Buffer::UpdateData
		DynamicStorage = 1 << 0,
		// Hints to the implementation to place the buffer storage in host memory
		ClientStorage = 1 << 1,
		// Maps the buffer (persistently and coherently) upon creation
		MapMemory = 1 << 2,
	};
	SE_DECLARE_FLAG_TYPE(BufferStorageFlags, BufferStorageFlag, uint32_t);

	struct BufferFillInfo final
	{
		uint64_t offset = 0;
		uint64_t size = WHOLE_BUFFER;
		uint32_t data = 0;
	};

	BufferPtr CreateBuffer(const void* data, size_t size, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");

	// Gets a pointer that is mapped to the buffer's data store
		// A pointer to mapped memory if the buffer was created with BufferStorageFlag::MAP_MEMORY, otherwise nullptr
	[[nodiscard]] void* GetMappedPointer(BufferPtr buffer) noexcept;
	[[nodiscard]] bool IsMapped(BufferPtr buffer) noexcept;
	[[nodiscard]] auto Size(BufferPtr buffer) noexcept;
	[[nodiscard]] auto Handle(BufferPtr buffer) noexcept;

	// Invalidates the content of the buffer's data store
	// This call can be used to optimize driver synchronization in certain cases.
	void Invalidate(BufferPtr buffer);

	void UpdateData(BufferPtr buffer, const void* data, size_t size, size_t destOffsetBytes = 0);
	void FillData(BufferPtr buffer, const BufferFillInfo& clear = {});

} // namespace gpu::buffer