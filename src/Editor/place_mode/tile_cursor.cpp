#include "stdafx.h"
#include "place_mode.hpp"
//=============================================================================
void ed::TileCursor::Update(const glm::vec3& gridPos)
{
	position = gridPos;
}
//=============================================================================
void ed::TileCursor::Draw(const gr::Camera& camera)
{
	(void)camera;
}
//=============================================================================
