#include "stdafx.h"
#include "app_mouseLook.h"
#include "math_point2.h"
#include "app_input.h"
#include "app_window.h"
//=============================================================================
// Called every frame from GameUpdate
void input::MouseLook::Update(gr::Camera& cam)
{
	// Auto-release on focus loss or minimize
	if (m_captured && !canCapture())
	{
		endCapture();
		return;
	}

	// Auto-recapture when focus regained (if button still held)
	if (!m_captured && m_wantCapture && canCapture())
	{
		beginCapture();
		if (!m_captured) return;
	}

	if (!m_captured) return;

	// Compute raw mouse delta since last warp
	math::point2 current = input::GetMousePosition();
	int dx = current.x - m_centerX;
	int dy = current.y - m_centerY;

	// switch center to window center so player has room to turn both ways
	m_centerX = window::GetWidth() / 2;
	m_centerY = window::GetHeight() / 2;

	// Warp cursor back to new center
	input::SetMousePosition(m_centerX, m_centerY);

	// Apply camera rotation (scale by sensitivity)
	if (dx != 0 || dy != 0)
		cam.Rotate(-static_cast<float>(dy) * 0.1f, static_cast<float>(dx) * 0.1f, 0.0f);
}
//=============================================================================
// Call when right mouse button goes down
void input::MouseLook::OnRightDown()
{
	m_wantCapture = true;
	if (canCapture())
		beginCapture();
}
//=============================================================================
// Call when right mouse button goes up
void input::MouseLook::OnRightUp()
{
	m_wantCapture = false;
	endCapture();
}
//=============================================================================
// Drop everything (e.g. on game close)
void input::MouseLook::Reset()
{
	m_wantCapture = false;
	endCapture();
}
//=============================================================================
bool input::MouseLook::canCapture() const
{
	return window::GetWindowActive() && !window::GetWindowMinimized();
}
//=============================================================================
void input::MouseLook::beginCapture()
{
	if (m_captured) return;

	// use current cursor position as center so dx/dy are zero on this frame
	math::point2 current = input::GetMousePosition();
	m_centerX = current.x;
	m_centerY = current.y;

	// Hide cursor, capture mouse to window, clip to client area
	input::CaptureMouse(true);
	// Center cursor so first delta is zero
	input::SetMousePosition(m_centerX, m_centerY);

	m_captured = true;
}
//=============================================================================
void input::MouseLook::endCapture()
{
	if (!m_captured) return;
	m_captured = false;

	input::CaptureMouse(false);
}
//=============================================================================