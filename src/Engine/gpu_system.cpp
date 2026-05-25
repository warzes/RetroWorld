#include "stdafx.h"
#include "gpu_system.h"
#include "gpu_core.h"
#include "gpu_program.h"
#include "app_window.h"
#include "core_log.h"
//=============================================================================
namespace
{
	bool vSync{ false };
}
//=============================================================================
bool gpu::Init(const CreateInfo& createInfo)
{
	vSync = createInfo.vSync;
	//windowSize.width = window::GetWidth();
	//windowSize.height = window::GetHeight();

	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	core::Print("Vendor: " + std::string(vendor));
	core::Print("Renderer: " + std::string(renderer));
	core::Print("OpenGL version supported: " + std::string(version));

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	MetricsCurrent = MetricsPrevious = { 0 };

	gpu::program::BindShaderProgram(nullptr);

	return true;
}
//=============================================================================
void gpu::Close()
{
}
//=============================================================================
bool gpu::BeginFrame()
{
	if (window::GetWindowMinimized())
		return false;

	//if (windowSize.width != window::GetWidth() || windowSize.height != window::GetHeight())
	//{
	//	windowSize.width = window::GetWidth();
	//	windowSize.height = window::GetHeight();
	//}

	MetricsPrevious = MetricsCurrent;
	MetricsCurrent = { 0 };

	return true;
}
//=============================================================================
void gpu::EndFrame()
{

}
//=============================================================================