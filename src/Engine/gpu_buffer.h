#pragma once

#include "core_flags.h"

namespace gpu::buffer
{
	struct Buffer;
	using BufferPtr = std::shared_ptr<Buffer>;

	constexpr inline uint64_t WHOLE_BUFFER = static_cast<uint64_t>(-1);

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

	class ByteSpan final : public std::span<const std::byte>
	{
	public:
		template<typename T> requires std::is_trivially_copyable_v<T>
		ByteSpan(const T& t) : std::span<const std::byte>(std::as_bytes(std::span{ &t, static_cast<size_t>(1) })) {}
		template<typename T> requires std::is_trivially_copyable_v<T>
		ByteSpan(std::span<const T> t) : std::span<const std::byte>(std::as_bytes(t)) {}
		template<typename T> requires std::is_trivially_copyable_v<T>
		ByteSpan(std::span<T> t) : std::span<const std::byte>(std::as_bytes(t)) {}
	};

	[[nodiscard]] BufferPtr CreateBuffer(size_t size, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");
	[[nodiscard]] BufferPtr CreateBuffer(ByteSpan data, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");
	[[nodiscard]] BufferPtr CreateBuffer(const void* data, size_t size, BufferStorageFlags storageFlags = BufferStorageFlag::None, std::string_view name = "");

	// Gets the buffer's persistently mapped memory as a span of bytes.
	// Returns an empty span if the buffer was not created with BufferStorageFlag::MapMemory.
	[[nodiscard]] std::span<std::byte> GetMappedPointer(const BufferPtr& buffer) noexcept;
	[[nodiscard]] bool IsMapped(const BufferPtr& buffer) noexcept;
	[[nodiscard]] size_t Size(const BufferPtr& buffer) noexcept;
	[[nodiscard]] uint32_t Handle(const BufferPtr& buffer) noexcept;
	[[nodiscard]] bool IsValid(const BufferPtr& buffer) noexcept;

	// Invalidates the content of the buffer's data store
	// This call can be used to optimize driver synchronization in certain cases.
	void Invalidate(const BufferPtr& buffer);

	void FillData(const BufferPtr& buffer, const BufferFillInfo& clear = {});
	void UpdateData(const BufferPtr& buffer, ByteSpan data, size_t destOffsetBytes = 0);
	void UpdateData(const BufferPtr& buffer, const void* data, size_t size, size_t destOffsetBytes = 0);

	template<class T> requires(std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
	void UpdateData(const BufferPtr& buffer, const T& data, size_t startIndex = 0)
	{
		UpdateData(buffer, ByteSpan(data), sizeof(T) * startIndex);
	}
	template<class T> requires(std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
	void UpdateData(const BufferPtr& buffer, std::span<const T> data, size_t startIndex = 0)
	{
		UpdateData(buffer, ByteSpan(data), sizeof(T) * startIndex);
	}

} // namespace gpu::buffer