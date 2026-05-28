#pragma once

namespace gpu
{
	// An object used for CPU-GPU synchronization
	class Fence
	{
	public:
		explicit Fence() noexcept = default;
		Fence(Fence&& old) noexcept;
		Fence& operator=(Fence&& old) noexcept;
		Fence(const Fence&) = delete;
		Fence& operator=(const Fence&) = delete;
		~Fence();

		// Inserts a fence into the command stream
		void Signal();

		// Waits for the fence to be signaled and returns
		// @return How long (in nanoseconds) the fence blocked
		/// @todo Add timeout parameter
		uint64_t Wait();

	private:
		void deleteSync();

		void* m_sync{};
	};
} // namespace gpu