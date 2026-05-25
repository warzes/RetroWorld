#include "stdafx.h"
#if USE_OPENGL && PLATFORM_DESKTOP_WIN32
#include "core_log.h"
#include "_ogl_context.h"
#if defined(_MSC_VER)
#	pragma comment( lib, "OpenGL32.lib" )
#endif
//=============================================================================
#ifdef _WIN32
extern "C"
{
	__declspec(dllexport) unsigned long NvOptimusEnablement = 1;
	__declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif
//=============================================================================
void APIENTRY GLDebugMessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) noexcept
{
	if (id == 131169 ||
		id == 131185 || // NV: Buffer will use video memory
		id == 131218 ||
		id == 131204 || // Texture cannot be used for texture mapping
		id == 131222 ||
		id == 131154 || // NV: pixel transfer is synchronized with 3D rendering
		id == 0         // gl{Push, Pop}DebugGroup
		)
		return;

	std::string msg;

	msg = "Debug message (" + std::to_string(id) + "): " + message + "\n";

	switch (source)
	{
	case GL_DEBUG_SOURCE_API:             msg += "Source: API\n"; break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   msg += "Source: Window System\n"; break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER: msg += "Source: Shader Compiler\n"; break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:     msg += "Source: Third Party\n"; break;
	case GL_DEBUG_SOURCE_APPLICATION:     msg += "Source: Application\n"; break;
	case GL_DEBUG_SOURCE_OTHER:           msg += "Source: Other\n"; break;
	}

	switch (type)
	{
	case GL_DEBUG_TYPE_ERROR:               msg += "Type: Error\n"; break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: msg += "Type: Deprecated Behaviour\n"; break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  msg += "Type: Undefined Behaviour\n"; break;
	case GL_DEBUG_TYPE_PORTABILITY:         msg += "Type: Portability\n"; break;
	case GL_DEBUG_TYPE_PERFORMANCE:         msg += "Type: Performance\n"; break;
	case GL_DEBUG_TYPE_MARKER:              msg += "Type: Marker\n"; break;
	case GL_DEBUG_TYPE_PUSH_GROUP:          msg += "Type: Push Group\n"; break;
	case GL_DEBUG_TYPE_POP_GROUP:           msg += "Type: Pop Group\n"; break;
	case GL_DEBUG_TYPE_OTHER:               msg += "Type: Other\n"; break;
	}
	switch (severity)
	{
	case GL_DEBUG_SEVERITY_HIGH:         msg += "Severity: High"; break;
	case GL_DEBUG_SEVERITY_MEDIUM:       msg += "Severity: Medium"; break;
	case GL_DEBUG_SEVERITY_LOW:          msg += "Severity: Low"; break;
	case GL_DEBUG_SEVERITY_NOTIFICATION: msg += "Severity: Notification"; break;
	}
	core::Error(msg);
}
//=============================================================================
bool InitOpenGLContext(HINSTANCE hinstance, HWND hwnd, HDC hdc, HGLRC& outcontext, bool adaptiveVsync)
{
	const uint32_t pfdFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;

	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = DefWindowProc;
	wc.hInstance = hinstance;
	wc.lpszClassName = L"Dummy3DWindowClass";
	if (!RegisterClass(&wc))
	{
		core::Error("Failed to register dummy window class");
		return false;
	}

	// dummy window
	{
		struct DummyData final
		{
			~DummyData()
			{
				if (context)
				{
					wglMakeCurrent(nullptr, nullptr);
					wglDeleteContext(context);
				}
				if (dc) ReleaseDC(win, dc);
				if (win) DestroyWindow(win);
				UnregisterClass(L"Dummy3DWindowClass", hinstance);
			}

			HWND win{ nullptr };
			HDC dc{ nullptr };
			HGLRC context{ nullptr };
			HINSTANCE hinstance{ nullptr };
		} dummy;

		dummy.hinstance = hinstance;

		dummy.win = CreateWindow(wc.lpszClassName, L"Dummy Window", 0, 0, 0, 0, 0, 0, 0, hinstance, 0);
		if (!dummy.win)
		{
			core::Error("Failed to create dummy window");
			return false;
		}

		dummy.dc = GetDC(dummy.win);
		if (!dummy.dc)
		{
			core::Error("Failed to get device context for dummy window");
			return false;
		}

		{
			PIXELFORMATDESCRIPTOR pfd{};
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pfd.nVersion = 1;
			pfd.dwFlags = pfdFlags;
			pfd.iPixelType = PFD_TYPE_RGBA;
			pfd.iLayerType = PFD_MAIN_PLANE;
			pfd.cColorBits = 32;
			pfd.cAlphaBits = 8;
			pfd.cDepthBits = 24;
			pfd.cStencilBits = 8;
			pfd.cAuxBuffers = 0;

			int pixelFormat = ChoosePixelFormat(dummy.dc, &pfd);
			if (pixelFormat == 0)
			{
				core::Error("Failed to choose pixel format");
				return false;
			}

			if (!SetPixelFormat(dummy.dc, pixelFormat, &pfd))
			{
				core::Error("Failed to set pixel format");
				return false;
			}
		}

		dummy.context = wglCreateContext(dummy.dc);
		if (!dummy.context)
		{
			core::Error("Failed to create OpenGL context");
			return false;
		}
		if (!wglMakeCurrent(dummy.dc, dummy.context))
		{
			core::Error("Failed to make OpenGL context current");
			return false;
		}
		if (!gladLoaderLoadWGL(dummy.dc)) // TODO: заменить на свое
		{
			core::Error("Failed to load WGL functions");
			return false;
		}
	}

	const int attribs[] = {
#if USE_SRGB
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
#endif
		WGL_DRAW_TO_WINDOW_ARB,           GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,           GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,            GL_TRUE,
		WGL_ACCELERATION_ARB,             WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,               WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,               32,
		WGL_DEPTH_BITS_ARB,               24,
		WGL_STENCIL_BITS_ARB,             8,
		0
	};

	int pixelFormat{ 0 };
	UINT numFormats;
	BOOL result = wglChoosePixelFormatARB(hdc, attribs, 0, 1, &pixelFormat, &numFormats);
	if (result == FALSE || !numFormats || !pixelFormat)
	{
		core::Error("Failed to create a pixel format for WGL.");
		return false;
	}

	PIXELFORMATDESCRIPTOR pfd{};
	if (!DescribePixelFormat(hdc, pixelFormat, sizeof(pfd), &pfd) ||
		!SetPixelFormat(hdc, pixelFormat, &pfd))
	{
		core::Error("Failed to set the WGL pixel format");
		return false;
	}

	const int contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 6,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		WGL_CONTEXT_FLAGS_ARB,
#if defined(_DEBUG)
		WGL_CONTEXT_DEBUG_BIT_ARB,
#else
		GLAD_WGL_ARB_create_context_no_error ? WGL_CONTEXT_OPENGL_NO_ERROR_ARB : 0,
#endif
		0, 0
	};
	outcontext = wglCreateContextAttribsARB(hdc, nullptr, contextAttribs);
	if (!outcontext)
	{
		core::Error("Failed to create OpenGL context with WGL.");
		return false;
	}
	wglMakeCurrent(hdc, outcontext);

	if (!gladLoaderLoadGL())
	{
		core::Error("Failed to load OpenGL functions");
		return false;
	}

	// enable debug context
	int flags;
	glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
	if (flags & GL_CONTEXT_FLAG_DEBUG_BIT)
	{
		core::Print("Enable OpenGL Debug Context");
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // makes sure errors are displayed synchronously
		glDebugMessageCallback(GLDebugMessageCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
	}

	wglSwapIntervalEXT(adaptiveVsync ? -1 : 0); // Adaptive Vsync, если есть EXT_swap_control_tear (включает vsync если фпс высокое, отключает на низком)

	return true;
}
#endif // USE_OPENGL && PLATFORM_DESKTOP_WIN32
//=============================================================================