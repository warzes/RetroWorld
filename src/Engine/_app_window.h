#pragma once

#include "app_window.h"

namespace window
{
	bool Init(const WindowCreateInfo& createInfo);
	void Close();

	bool IsShouldClose() noexcept;
	bool PollEvents();
	void Swap() noexcept;

	HWND GetHwnd();
	HINSTANCE GetInstance();
} // namespace app::window