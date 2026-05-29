#pragma once

#include "app_keys.h"
#include "math_point2.h"

namespace input
{
	bool IsKeyDown(KeyboardType key);
	bool IsKeyUp(KeyboardType key);

	bool IsMouseDown(MouseType type);
	bool IsMouseUp(MouseType type);
	const math::point2& GetMousePosition();
	float GetMouseDelta();
	bool IsMouseMoving();

	void SetCursorVisible(bool visible);
	bool SetMousePosition(int x, int y);

	void CaptureMouse(bool capture);

} // namespace input