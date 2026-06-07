#include "stdafx.h"
#include "place_mode.hpp"
#include "../map_man/map_man.hpp"
//=============================================================================
ed::BrushCursor::BrushCursor(MapMan& mapMan)
	: brush(mapMan, 1, 1, 1)
{}
//=============================================================================
void ed::BrushCursor::Update(const glm::vec3& gridPos)
{
	position = gridPos;
}
//=============================================================================
void ed::BrushCursor::Draw(const gr::Camera& camera)
{
	(void)camera;
	// Cursor is drawn via scene graph in PlaceMode::Update()
}
//=============================================================================
