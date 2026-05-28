#include "stdafx.h"
#include "app_window.h"
#include "_app_window.h"
#include "core_log.h"
#include "_app_input.h"
#include "app_messageHandler.h"
//=============================================================================
#ifndef GET_X_LPARAM
#	define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#	define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
//=============================================================================
namespace
{
	constexpr const wchar_t* AppWindowClass = L"SEGameWindow";

	HWND      hwnd{ nullptr };
	HINSTANCE instance{ nullptr };
	HDC       hdc{ nullptr };
	HGLRC     context{ nullptr };
	bool      isShouldClose{ true };
	uint16_t  windowWidth{ 0 };
	uint16_t  windowHeight{ 0 };
	float     windowAspect{ 1.0f };
	bool      windowActive{ true }; // Starts active

	bool      resizing{ false };
	bool      windowMinimized{ false };

	MSG       msg = {};
}
//=============================================================================
extern app::MessageHandler* userMessageHandler;
//=============================================================================
static void windowSetSize(uint16_t w, uint16_t h) noexcept
{
	windowWidth = std::max(w, static_cast<uint16_t>(1));
	windowHeight = std::max(h, static_cast<uint16_t>(1));
	windowAspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
}
//=============================================================================
uint16_t window::GetWidth() noexcept
{
	return windowWidth;
}
//=============================================================================
uint16_t window::GetHeight() noexcept
{
	return windowHeight;
}
//=============================================================================
float window::GetAspectRatio() noexcept
{
	return windowAspect;
}
//=============================================================================
bool window::GetWindowMinimized() noexcept
{
	return windowMinimized;
}
//=============================================================================
bool window::GetWindowActive() noexcept
{
	return windowActive;
}
//=============================================================================
HWND window::GetHwnd()
{
	return hwnd;
}
//=============================================================================
// Main message handler for the sample.
static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) noexcept
{
	extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return 0;

	switch (message)
	{
	case WM_CLOSE:
		isShouldClose = true;
		if (userMessageHandler) userMessageHandler->OnWindowClose();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_ENTERSIZEMOVE:
		resizing = true;
		return 0;
	case WM_EXITSIZEMOVE:
		resizing = false;
		return 0;
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO minMaxInfo = (LPMINMAXINFO)lParam;
			minMaxInfo->ptMinTrackSize.x = 640;
			minMaxInfo->ptMinTrackSize.y = 360;
		}
		return 0;
	case WM_SIZE:
		windowMinimized = (wParam == SIZE_MINIMIZED);
		if ((wParam != SIZE_MINIMIZED))
		{
			if ((resizing) || ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)))
			{
				uint16_t width = static_cast<uint16_t>(LOWORD(lParam));
				uint16_t height = static_cast<uint16_t>(HIWORD(lParam));
				windowSetSize(width, height);
				if (userMessageHandler) userMessageHandler->OnSizeChanged(width, height);
			}
		}
		return 0;
	case WM_ACTIVATE:
		windowActive = (wParam != WA_INACTIVE);
		return 0;
	case WM_KILLFOCUS:
		windowActive = false;
		return 0;
	case WM_SETFOCUS:
		windowActive = true;
		return 0;
	case WM_KEYDOWN:
		{
			const int keycode = HIWORD(lParam) & 0x1FF;
			KeyboardType key = input::GetKeyFromKeyCode(keycode);
			input::OnKeyDown(key);
			return 0;
		}
	case WM_KEYUP:
		{
			const int keycode = HIWORD(lParam) & 0x1FF;
			KeyboardType key = input::GetKeyFromKeyCode(keycode);
			input::OnKeyUp(key);
			return 0;
		}

	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_XBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
	case WM_XBUTTONUP:
		{
			MouseType button = MouseType::MOUSE_BUTTON_LEFT;

			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
			math::point2 pos(x, y);

			if (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP)
			{
				button = MouseType::MOUSE_BUTTON_LEFT;
			}
			else if (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
			{
				button = MouseType::MOUSE_BUTTON_RIGHT;
			}
			else if (message == WM_MBUTTONDOWN || message == WM_MBUTTONUP)
			{
				button = MouseType::MOUSE_BUTTON_MIDDLE;
			}
			else if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
			{
				button = MouseType::MOUSE_BUTTON_4;
			}
			else
			{
				button = MouseType::MOUSE_BUTTON_5;
			}

			if (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN || message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN)
			{
				input::OnMouseDown(button, pos);
			}
			else
			{
				input::OnMouseUp(button, pos);
			}

			return 0;
		}

	case WM_MOUSEMOVE:
		{
			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
			math::point2 pos(x, y);
			input::OnMouseMove(pos);
			return 0;
		}
	case WM_MOUSEWHEEL:
		{
			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
			math::point2 pos((float)x, (float)y);
			input::OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, pos);
			return 0;
		}
	case WM_MOUSEHWHEEL:
		{
			const int x = GET_X_LPARAM(lParam);
			const int y = GET_Y_LPARAM(lParam);
			math::point2 pos((float)x, (float)y);
			input::OnMouseWheel((float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, pos);
			return 0;
		}
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}
//=============================================================================
bool window::Init(const WindowCreateInfo& createInfo)
{
	resizing = false;
	windowMinimized = false;
	isShouldClose = false;

	instance = GetModuleHandle(nullptr);

	WNDCLASSEX windowClass    = { 0 };
	windowClass.cbSize        = sizeof(WNDCLASSEX);
	windowClass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	windowClass.lpfnWndProc   = WindowProc;
	windowClass.hInstance     = instance;
	windowClass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = AppWindowClass;
	if (!RegisterClassEx(&windowClass))
	{
		core::Fatal("Failed to register window class");
		return false;
	}

	const DWORD winExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
	DWORD winStyle = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SIZEBOX;

	RECT windowRect = { 0, 0, 
		static_cast<LONG>(createInfo.width), 
		static_cast<LONG>(createInfo.height) };
	AdjustWindowRectEx(&windowRect, winStyle, FALSE, winExStyle);

	hwnd = CreateWindow(AppWindowClass, createInfo.title.c_str(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, instance, nullptr);
	if (!hwnd)
	{
		core::Error("Failed to create window");
		return false;
	}

#if USE_OPENGL
	hdc = GetDC(hwnd);
	if (!hdc)
	{
		core::Error("Failed to get device context");
		return false;
	}

	bool InitOpenGLContext(HINSTANCE, HWND, HDC, HGLRC&, bool);
	if (!InitOpenGLContext(instance, hwnd, hdc, context, createInfo.adaptiveVsync))
		return false;
#endif

	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	windowSetSize(
		static_cast<uint16_t>(clientRect.right - clientRect.left),
		static_cast<uint16_t>(clientRect.bottom - clientRect.top));

	// ImGui
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls
		io.IniFilename = nullptr;

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_InitForOpenGL(hwnd);
		ImGui_ImplOpenGL3_Init();
	}

	return true;
}
//=============================================================================
void window::Close()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#if USE_OPENGL
	gladLoaderUnloadGL();
#endif
	if (context)
	{
		wglMakeCurrent(nullptr, nullptr);
		wglDeleteContext(context);
	}
	if (hdc) ReleaseDC(hwnd, hdc);
	if (hwnd) DestroyWindow(hwnd);
	if (instance) UnregisterClass(AppWindowClass, instance);
	context = nullptr;
	hdc = nullptr;
	hwnd = nullptr;
	instance = nullptr;
	isShouldClose = true;
}
//=============================================================================
bool window::IsShouldClose() noexcept
{
	return isShouldClose;
}
//=============================================================================
bool window::PollEvents()
{
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if (msg.message == WM_QUIT)
		{
			isShouldClose = true;
			return false;
		}
	}

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	return true;
}
//=============================================================================
void window::Swap() noexcept
{
#if USE_OPENGL
	{
#if USE_SRGB
		glDisable(GL_FRAMEBUFFER_SRGB);
#endif
		ImGui::Render();
		auto* drawData = ImGui::GetDrawData();
		if (drawData->CmdListsCount > 0)
		{
			glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Draw ImGui");
			//rhi::detail::BindFBO(0);
			glViewport(0, 0, window::GetWidth(), window::GetHeight());
			ImGui_ImplOpenGL3_RenderDrawData(drawData);
			glPopDebugGroup();
		}
#if USE_SRGB
		glEnable(GL_FRAMEBUFFER_SRGB);
#endif
	}

	SwapBuffers(hdc);
#endif
}
//=============================================================================