#pragma once

#if USE_OPENGL && PLATFORM_DESKTOP_WIN32
bool InitOpenGLContext(HINSTANCE hinstance, HWND hwnd, HDC hdc, HGLRC& outcontext, bool adaptiveVsync);
#endif