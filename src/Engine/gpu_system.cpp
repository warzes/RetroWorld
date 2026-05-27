#include "stdafx.h"
#include "_gpu_system.h"
#include "gpu_program.h"
#include "gpu_cmd.h"
#include "gpu_vao.h"
#include "_gpu_contextState.h"
#include "_gpu_enumDesc.h"
#include "app_window.h"
#include "core_log.h"
//=============================================================================
namespace
{
}
//=============================================================================
bool gpu::Init()
{
	context.Init(window::GetWidth(), window::GetHeight());

	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	core::Print("Vendor: " + std::string(vendor));
	core::Print("Renderer: " + std::string(renderer));
	core::Print("OpenGL version supported: " + std::string(version));

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	MetricsCurrent = MetricsPrevious = { 0 };

	gpu::program::BindShaderProgram(nullptr);
	gpu::vao::BindVertexArray(nullptr);

	return true;
}
//=============================================================================
void gpu::Close()
{
	context.Clear();
}
//=============================================================================
bool gpu::BeginFrame()
{
	if (window::GetWindowMinimized())
		return false;

	context.BeginFrame();
	if (context.contextWidth != window::GetWidth() || context.contextHeight != window::GetHeight())
	{
		context.contextWidth = window::GetWidth();
		context.contextHeight = window::GetHeight();
		context.isContextResize = true;
	}

	MetricsPrevious = MetricsCurrent;
	MetricsCurrent = { 0 };

	return true;
}
//=============================================================================
void gpu::EndFrame()
{
	context.EndFrame();
}
//=============================================================================