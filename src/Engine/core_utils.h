#pragma once

namespace core
{
	template <typename T, std::size_t N>
	constexpr std::size_t CountOf(T const (&)[N]) noexcept
	{
		return N;
	}

	template <class C>
	std::size_t CountOf(C const& c)
	{
		return c.size();
	}
	
} // namespace core