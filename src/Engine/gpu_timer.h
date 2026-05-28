#pragma once

namespace gpu
{
	// Synchronous single-buffered GPU-timeline timer. Querying the timer will result in a stall as commands are flushed and waited on to complete
	// Use sparingly, and only if detailed perf data is needed for a particular draw.
	// @todo This class is in desparate need of an update
	class TimerQuery final
	{
	public:
		TimerQuery();
		~TimerQuery();

		TimerQuery(const TimerQuery&) = delete;
		TimerQuery& operator=(const TimerQuery&) = delete;
		TimerQuery(TimerQuery&& old) noexcept
		{
			m_queries[0] = std::exchange(old.m_queries[0], 0u);
			m_queries[1] = std::exchange(old.m_queries[1], 0u);
		}
		TimerQuery& operator=(TimerQuery&& old) noexcept
		{
			if (&old == this) return *this;
			this->~TimerQuery();
			return *new (this) TimerQuery(std::move(old));
		}

		// returns how (in ns) we blocked for (BLOCKS)
		uint64_t GetTimestamp();

	private:
		uint32_t m_queries[2];
	};

	// Async N-buffered timer query that does not induce pipeline stalls
	// Useful for measuring performance of passes every frame without causing stalls.
	// However, the results returned may be from multiple frames ago, and results are not guaranteed to be available. In practice, setting N to 5 should allow at least one query to be available every frame.
	class TimerQueryAsync final
	{
	public:
		TimerQueryAsync(uint32_t N);
		~TimerQueryAsync();

		TimerQueryAsync(const TimerQueryAsync&) = delete;
		TimerQueryAsync& operator=(const TimerQueryAsync&) = delete;
		TimerQueryAsync(TimerQueryAsync&& old) noexcept
			: m_start(std::exchange(old.m_start, 0u))
			, m_count(std::exchange(old.m_count, 0u))
			, m_capacity(std::exchange(old.m_capacity, 0u))
			, m_queries(std::exchange(old.m_queries, nullptr))
		{}
		TimerQueryAsync& operator=(TimerQueryAsync&& old) noexcept
		{
			if (&old == this)
				return *this;
			this->~TimerQueryAsync();
			return *new (this) TimerQueryAsync(std::move(old));
		}

		// Begins a query zone
		// @note EndZone must be called before another zone can begin
		void BeginZone();

		// Ends a query zone
		// @note BeginZone must be called before a zone can end
		void EndZone();

		// Gets the latest available query
		// @return The latest query, if available. Otherwise, std::nullopt is returned
		[[nodiscard]] std::optional<uint64_t> PopTimestamp();

	private:
		uint32_t m_start{}; // next timer to be used for measurement
		uint32_t m_count{}; // number of timers 'buffered', ie measurement was started by result not read yet
		uint32_t m_capacity{};
		uint32_t* m_queries{};
	};

	// RAII wrapper for TimerQueryAsync
	template<typename T>
	class TimerScoped
	{
	public:
		TimerScoped(T& zone) : m_zone(zone)
		{
			m_zone.BeginZone();
		}

		~TimerScoped()
		{
			m_zone.EndZone();
		}

	private:
		T& m_zone;
	};
} // namespace gpu