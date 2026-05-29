#pragma once

#include "gr_camera.h"

namespace input
{
	// === WinAPI mouse-look for first-person camera ===
	// Handles: focus loss (Alt+Tab), minimize, cursor clipping, synthetic WM_MOUSEMOVE filtering
	class MouseLook final
	{
	public:
		// Called every frame from GameUpdate
		void Update(gr::Camera& cam);

		// Call when right mouse button goes down
		void OnRightDown();

		// Call when right mouse button goes up
		void OnRightUp();

		// Drop everything (e.g. on game close)
		void Reset();

	private:
		bool canCapture() const;
		void beginCapture();
		void endCapture();

		bool m_captured = false;
		bool m_wantCapture = false;
		int  m_centerX = 0;
		int  m_centerY = 0;
	};
} // namespace input