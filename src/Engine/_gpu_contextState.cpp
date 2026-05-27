#include "stdafx.h"
#include "_gpu_contextState.h"
//=============================================================================
void gpu::ContextState::BeginFrame()
{
	isRendering = true;
}
//=============================================================================
void gpu::ContextState::EndFrame()
{
	currentVertexArray = nullptr;
	currentShaderProgram = nullptr;

	isContextResize = false;
	isRendering = false;
	isIndexBufferBound = false;
}
//=============================================================================
void gpu::ContextState::Clear()
{
	vertexArrayCache.clear();

	*this = { 0 };
}
//=============================================================================