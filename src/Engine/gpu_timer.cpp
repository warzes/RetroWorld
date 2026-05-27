#include "stdafx.h"
#include "gpu_timer.h"

namespace gpu
{
	TimerQuery::TimerQuery()
	{
		glGenQueries(2, m_queries);
		glQueryCounter(m_queries[0], GL_TIMESTAMP);
	}

	TimerQuery::~TimerQuery()
	{
		glDeleteQueries(2, m_queries);
	}

	uint64_t TimerQuery::GetTimestamp()
	{
		int complete = 0;
		glQueryCounter(m_queries[1], GL_TIMESTAMP);
		while (!complete)
			glGetQueryObjectiv(m_queries[1], GL_QUERY_RESULT_AVAILABLE, &complete);
		uint64_t startTime, endTime;
		glGetQueryObjectui64v(m_queries[0], GL_QUERY_RESULT, &startTime);
		glGetQueryObjectui64v(m_queries[1], GL_QUERY_RESULT, &endTime);
		std::swap(m_queries[0], m_queries[1]);
		return endTime - startTime;
	}

	TimerQueryAsync::TimerQueryAsync(uint32_t N) : m_capacity(N)
	{
		assert(m_capacity > 0);
		m_queries = new uint32_t[m_capacity * 2];
		glGenQueries(m_capacity * 2, m_queries);
	}

	TimerQueryAsync::~TimerQueryAsync()
	{
		glDeleteQueries(m_capacity * 2, m_queries);
		delete[] m_queries;
	}

	void TimerQueryAsync::BeginZone()
	{
		// begin a query if there is at least one inactive
		if (m_count < m_capacity)
		{
			glQueryCounter(m_queries[m_start], GL_TIMESTAMP);
		}
	}

	void TimerQueryAsync::EndZone()
	{
		// end a query if there is at least one inactive
		if (m_count < m_capacity)
		{
			glQueryCounter(m_queries[m_start + m_capacity], GL_TIMESTAMP);
			m_start = (m_start + 1) % m_capacity; // wrap
			m_count++;
		}
	}

	std::optional<uint64_t> TimerQueryAsync::PopTimestamp()
	{
		// return nothing if there is no active query
		if (m_count == 0)
		{
			return std::nullopt;
		}

		// get the index of the oldest query
		uint32_t index = (m_start + m_capacity - m_count) % m_capacity;

		// getting the start result is a sanity check
		GLint startResultAvailable{};
		GLint endResultAvailable{};
		glGetQueryObjectiv(m_queries[index], GL_QUERY_RESULT_AVAILABLE, &startResultAvailable);
		glGetQueryObjectiv(m_queries[index + m_capacity], GL_QUERY_RESULT_AVAILABLE, &endResultAvailable);

		// the oldest query's result is not available, abandon ship!
		if (startResultAvailable == GL_FALSE || endResultAvailable == GL_FALSE)
		{
			return std::nullopt;
		}

		// pop oldest timing and retrieve result
		m_count--;
		uint64_t startTimestamp{};
		uint64_t endTimestamp{};
		glGetQueryObjectui64v(m_queries[index], GL_QUERY_RESULT, &startTimestamp);
		glGetQueryObjectui64v(m_queries[index + m_capacity], GL_QUERY_RESULT, &endTimestamp);
		return endTimestamp - startTimestamp;
	}
} // namespace rhi