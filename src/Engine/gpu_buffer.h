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

	class TriviallyCopyableByteSpan final : public std::span<const std::byte>
	{
	public:
		template<typename T> requires std::is_trivially_copyable_v<T>
		TriviallyCopyableByteSpan(const T& t) : std::span<const std::byte>(std::as_bytes(std::span{ &t, static_cast<size_t>(1) })) {}
		template<typename T> requires std::is_trivially_copyable_v<T>
		TriviallyCopyableByteSpan(std::span<const T> t) : std::span<const std::byte>(std::as_bytes(t)) {}
		template<typename T> requires std::is_trivially_copyable_v<T>
		TriviallyCopyableByteSpan(std::span<T> t) : std::span<const std::byte>(std::as_bytes(t)) {}
	};

	BufferPtr CreateBuffer(TriviallyCopyableByteSpan data, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");
	BufferPtr CreateBuffer(const void* data, size_t size, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");

	// Gets a pointer that is mapped to the buffer's data store
		// A pointer to mapped memory if the buffer was created with BufferStorageFlag::MAP_MEMORY, otherwise nullptr
	[[nodiscard]] void* GetMappedPointer(BufferPtr buffer) noexcept;
	[[nodiscard]] bool IsMapped(BufferPtr buffer) noexcept;
	[[nodiscard]] size_t Size(BufferPtr buffer) noexcept;
	[[nodiscard]] uint32_t Handle(BufferPtr buffer) noexcept;

	// Invalidates the content of the buffer's data store
	// This call can be used to optimize driver synchronization in certain cases.
	void Invalidate(BufferPtr buffer);

	void UpdateData(BufferPtr buffer, TriviallyCopyableByteSpan data, size_t destOffsetBytes = 0);
	void UpdateData(BufferPtr buffer, const void* data, size_t size, size_t destOffsetBytes = 0);
	void FillData(BufferPtr buffer, const BufferFillInfo& clear = {});

} // namespace gpu::buffer