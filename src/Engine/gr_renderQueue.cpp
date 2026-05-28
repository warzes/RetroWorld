#include "stdafx.h"
#include "gr_renderQueue.h"
//=============================================================================
void gr::RenderQueue::Sort()
{
	// Opaque: sort by materialId, then front-to-back (distance asc)
	std::ranges::sort(opaqueItems, [](const RenderItem& a, const RenderItem& b) {
		if (a.materialId != b.materialId)
			return a.materialId < b.materialId;
		return a.distanceToCamera < b.distanceToCamera;
		});

	// Transparent: sort back-to-front (distance desc)
	std::ranges::sort(transparentItems, [](const RenderItem& a, const RenderItem& b) {
		return a.distanceToCamera > b.distanceToCamera;
		});
}
//=============================================================================
void gr::RenderQueue::Clear()
{
	opaqueItems.clear();
	transparentItems.clear();
}
//=============================================================================
void gr::RenderQueue::Submit(const RenderItem& item, bool isTransparent)
{
	if (isTransparent)
		transparentItems.push_back(item);
	else
		opaqueItems.push_back(item);
}
//=============================================================================