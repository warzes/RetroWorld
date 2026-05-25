#include "stdafx.h"
#include "gpu_system.h"
#include "app_window.h"
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

	return true;
}
//=============================================================================
void gpu::EndFrame()
{

}
//=============================================================================