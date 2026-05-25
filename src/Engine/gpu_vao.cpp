#include "stdafx.h"
#include "gpu_vao.h"
#include "core_log.h"
//=============================================================================
gpu::vao::VertexArrayPtr currentVertexArray{ nullptr };
//=============================================================================
struct gpu::vao::VertexArray final
{
	VertexArray()
	{
		core::Debug("Create vertex array " + std::to_string(id));
		glCreateVertexArrays(1, &id);
	}
	~VertexArray()
	{
		if (id)
		{
			if (currentVertexArray->id == id) currentVertexArray = nullptr;
			core::Debug("Destroy vertex array " + std::to_string(id));
			glDeleteVertexArrays(1, &id);
		}
	}

	VertexArray(const VertexArray&) = delete;
	VertexArray& operator=(const VertexArray&) = delete;
	VertexArray(VertexArray&&) noexcept = default;
	VertexArray& operator=(VertexArray&&) noexcept = default;

	void Bind()
	{
#if defined(_DEBUG)
		core::Debug("Bind vertex array " + std::to_string(id));
#endif
		glBindVertexArray(id);
		MetricsCurrent.vertexBindings++;
	}

	operator bool() const noexcept { return id > 0; }
	unsigned GetId() const noexcept { return id; }
	bool IsValid() const noexcept { return id > 0; }

	uint32_t id{ 0 };
};
//=============================================================================
gpu::vao::VertexArrayPtr gpu::vao::CreateVertexArray()
{
	auto vao = std::make_shared<VertexArray>();
	return vao;
}
//=============================================================================
void gpu::vao::BindVertexArray(VertexArrayPtr vao)
{
	if (currentVertexArray != vao) // TODO: не только биндить текущий ресурс, но и текущий номер кадра, чтобы в новом кадре все заново биндилось
	{
		currentVertexArray = vao;
		vao ? vao->Bind() : glBindVertexArray(0);
	}
}
//=============================================================================