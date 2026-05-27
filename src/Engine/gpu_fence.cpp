#include "stdafx.h"
#include "gpu_fence.h"

namespace gpu
{
	Fence::Fence() {}

	Fence::~Fence()
	{
		DeleteSync();
	}

	Fence::Fence(Fence&& old) noexcept : m_sync(std::exchange(old.m_sync, nullptr)) {}

	Fence& Fence::operator=(Fence&& old) noexcept
	{
		if (this == &old)
			return *this;
		this->~Fence();
		return *new (this) Fence(std::move(old));
	}

	void Fence::Signal()
	{
		assert(m_sync == nullptr);
		m_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}

	uint64_t Fence::Wait()
	{
		assert(m_sync != nullptr);
		GLuint id;
		glGenQueries(1, &id);
		glBeginQuery(GL_TIME_ELAPSED, id);
		GLenum result = glClientWaitSync(reinterpret_cast<GLsync>(m_sync),
			GL_SYNC_FLUSH_COMMANDS_BIT,
			std::numeric_limits<GLuint64>::max());
		assert(result == GL_CONDITION_SATISFIED);
		glEndQuery(GL_TIME_ELAPSED);
		uint64_t elapsed;
		glGetQueryObjectui64v(id, GL_QUERY_RESULT, &elapsed);
		glDeleteQueries(1, &id);
		DeleteSync();
		return elapsed;
	}

	void Fence::DeleteSync()
	{
		glDeleteSync(reinterpret_cast<GLsync>(m_sync));
		m_sync = nullptr;
	}
} // namespace rhi