#include "stdafx.h"
#include "gpu_core.h"
//=============================================================================
gpu::ScopedDebugMarker::ScopedDebugMarker(const char* message)
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, message);
}
//=============================================================================
gpu::ScopedDebugMarker::~ScopedDebugMarker()
{
	glPopDebugGroup();
}
//=============================================================================