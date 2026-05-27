#pragma once

#include "gpu_core.h"

namespace gpu::buffer
{
	struct Buffer;
	using BufferPtr = std::shared_ptr<Buffer>;
	
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

		template<typename T> requires std::is_trivially_copyable_v<T>
		TriviallyCopyableByteSpan(const std::vector<T>& t) : std::span<const std::byte>(std::as_bytes(std::span{ t.data(), t.size()})) {}
	};

	BufferPtr CreateBuffer(size_t size, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");
	BufferPtr CreateBuffer(TriviallyCopyableByteSpan data, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");
	BufferPtr CreateBuffer(const void* data, size_t size, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");

	// Gets a pointer that is mapped to the buffer's data store
		// A pointer to mapped memory if the buffer was created with BufferStorageFlag::MAP_MEMORY, otherwise nullptr
	[[nodiscard]] void* GetMappedPointer(BufferPtr buffer) noexcept;
	[[nodiscard]] bool IsMapped(BufferPtr buffer) noexcept;
	[[nodiscard]] size_t Size(BufferPtr buffer) noexcept;
	[[nodiscard]] uint32_t Handle(BufferPtr buffer) noexcept;
	[[nodiscard]] bool IsValid(BufferPtr buffer) noexcept;

	// Invalidates the content of the buffer's data store
	// This call can be used to optimize driver synchronization in certain cases.
	void Invalidate(BufferPtr buffer);

	void FillData(BufferPtr buffer, const BufferFillInfo& clear = {});
	void UpdateData(BufferPtr buffer, TriviallyCopyableByteSpan data, size_t destOffsetBytes = 0);
	void UpdateData(BufferPtr buffer, const void* data, size_t size, size_t destOffsetBytes = 0);

	template<class T> requires(std::is_trivially_copyable_v<T>)
	void UpdateData(BufferPtr buffer, const T& data, size_t startIndex = 0)
	{
		UpdateData(buffer, data, sizeof(T) * startIndex);
	}
	template<class T> requires(std::is_trivially_copyable_v<T>)
	void UpdateData(BufferPtr buffer, std::span<const T> data, size_t startIndex = 0)
	{
		UpdateData(buffer, data, sizeof(T) * startIndex);
	}

} // namespace gpu::buffer