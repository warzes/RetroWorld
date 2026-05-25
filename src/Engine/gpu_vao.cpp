#include "stdafx.h"
#include "gpu_vao.h"
#include "_gpu_contextState.h"
#include "core_log.h"
#include "core_utils.h"
//=============================================================================
std::unordered_map<size_t, gpu::vao::VertexArrayPtr> vertexArrayCache;
//=============================================================================
gpu::vao::VertexArrayPtr currentVertexArray{ nullptr };
//=============================================================================
struct gpu::vao::VertexArray final
{
	VertexArray()
	{
		core::Debug("Created vertex array with handle " + std::to_string(id));
		glCreateVertexArrays(1, &id);
	}
	~VertexArray()
	{
		if (id)
		{
			if (currentVertexArray && currentVertexArray->id == id) currentVertexArray = nullptr;
			core::Debug("Destroyed vertex array with handle " + std::to_string(id));
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
inline size_t vertexInputStateHash(const std::vector<gpu::vao::VertexInputBindingDescription>& k)
{
	size_t hashVal{};

	for (const auto& desc : k)
	{
		auto cctup = std::make_tuple(desc.location, desc.binding, desc.format, desc.offset);
		auto chashVal = core::Hash<decltype(cctup)>{}(cctup);
		core::HashCombine(hashVal, chashVal);
	}

	return hashVal;
}
//=============================================================================
gpu::vao::VertexArrayPtr gpu::vao::CreateVertexArray(const std::vector<VertexInputBindingDescription>& inputState)
{
	auto inputHash = vertexInputStateHash(inputState);
	if (auto it = vertexArrayCache.find(inputHash); it != vertexArrayCache.end())
		return it->second;

	auto vao = std::make_shared<VertexArray>();
	for (uint32_t i = 0; i < inputState.size(); i++)
	{
		const auto& desc = inputState[i];
		glEnableVertexArrayAttrib(vao->id, desc.location);
		glVertexArrayAttribBinding(vao->id, desc.location, desc.binding);

		auto type         = FormatToTypeGL(desc.format);
		auto size         = FormatToSizeGL(desc.format);
		auto normalized   = IsFormatNormalizedGL(desc.format);
		auto internalType = FormatToFormatClass(desc.format);
		switch (internalType)
		{
		case GlFormatClass::FLOAT: glVertexArrayAttribFormat( vao->id, desc.location, size, type, normalized, desc.offset); break;
		case GlFormatClass::INT:   glVertexArrayAttribIFormat(vao->id, desc.location, size, type, desc.offset); break;
		case GlFormatClass::LONG:  glVertexArrayAttribLFormat(vao->id, desc.location, size, type, desc.offset); break;
		default: std::unreachable();
		}
	}

	return vertexArrayCache.insert({ inputHash, vao }).first->second;
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
void gpu::vao::BindVertexBuffer(VertexArrayPtr vao, uint32_t bindingIndex, gpu::buffer::BufferPtr buffer, uint64_t offset, uint64_t stride)
{
	//assert(context.isRendering);

	glVertexArrayVertexBuffer(vao->id,
		bindingIndex,
		gpu::buffer::Handle(buffer),
		static_cast<GLintptr>(offset),
		static_cast<GLsizei>(stride));
}
//=============================================================================
void gpu::vao::BindIndexBuffer(VertexArrayPtr vao, gpu::buffer::BufferPtr buffer, IndexType indexType)
{
	//assert(context.isRendering);

	context.isIndexBufferBound = true;
	context.currentIndexType = indexType;
	glVertexArrayElementBuffer(vao->id, gpu::buffer::Handle(buffer));
}
//=============================================================================