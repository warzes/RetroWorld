#include "stdafx.h"
#include "place_mode.hpp"
//=============================================================================
void ed::EntCursor::Update(const glm::vec3& gridPos)
{
	position = gridPos;
}
//=============================================================================
void ed::EntCursor::Draw(const gr::Camera& camera)
{
	(void)camera;
}
//=============================================================================
