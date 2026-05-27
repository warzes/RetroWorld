#include "stdafx.h"
#include "_gpu_system.h"
#include "gpu_program.h"
#include "gpu_vao.h"
#include "_gpu_contextState.h"
#include "_gpu_enumDesc.h"
#include "app_window.h"
#include "core_log.h"
//=============================================================================
namespace
{
}
//=============================================================================
bool gpu::Init()
{
	context.contextWidth  = window::GetWidth();
	context.contextHeight = window::GetHeight();

	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	core::Print("Vendor: " + std::string(vendor));
	core::Print("Renderer: " + std::string(renderer));
	core::Print("OpenGL version supported: " + std::string(version));

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	MetricsCurrent = MetricsPrevious = { 0 };

	gpu::program::BindShaderProgram(nullptr);
	gpu::vao::BindVertexArray(nullptr);

	return true;
}
//=============================================================================
void gpu::Close()
{
	context.Clear();
}
//=============================================================================
bool gpu::BeginFrame()
{
	if (window::GetWindowMinimized())
		return false;

	context.BeginFrame();
	if (context.contextWidth != window::GetWidth() || context.contextHeight != window::GetHeight())
	{
		context.contextWidth = window::GetWidth();
		context.contextHeight = window::GetHeight();
		context.isContextResize = true;
	}

	MetricsPrevious = MetricsCurrent;
	MetricsCurrent = { 0 };

	return true;
}
//=============================================================================
void gpu::EndFrame()
{
	context.EndFrame();
}
//=============================================================================
void gpu::SetClearColor(float red, float green, float blue, float alpha)
{
	glClearColor(red, green, blue, alpha);
}
//=============================================================================
void gpu::Clear(bool colorBuffer, bool depthBuffer, bool stencilBuffer)
{
	GLbitfield clearMask = 0;
	if (colorBuffer) clearMask |= GL_COLOR_BUFFER_BIT;
	if (depthBuffer) clearMask |= GL_DEPTH_BUFFER_BIT;
	if (stencilBuffer) clearMask |= GL_STENCIL_BUFFER_BIT;

	if (clearMask != 0)
	{
		glClear(clearMask);
	}
}
//=============================================================================
void gpu::SetCapability(RenderingCapability capability, bool value)
{
	(value ? glEnable : glDisable)(EnumToValue(capability));
}
//=============================================================================
bool gpu::GetCapability(RenderingCapability capability)
{
	return glIsEnabled(EnumToValue(capability)) == GL_TRUE;
}
//=============================================================================
void gpu::SetRasterizationLinesWidth(float width)
{
	glLineWidth(width);
}
//=============================================================================
void gpu::SetRasterizationMode(RasterizationMode rasterizationMode)
{
	glPolygonMode(GL_FRONT_AND_BACK, EnumToValue(rasterizationMode));
}
//=============================================================================
void gpu::SetStencilAlgorithm(CompareOp algorithm, int32_t reference, uint32_t mask)
{
	glStencilFunc(EnumToValue(algorithm), reference, mask);
}
//=============================================================================
void gpu::SetDepthAlgorithm(CompareOp algorithm)
{
	glDepthFunc(EnumToValue(algorithm));
}
//=============================================================================
void gpu::SetStencilMask(uint32_t mask)
{
	glStencilMask(mask);
}
//=============================================================================
void gpu::SetStencilOperations(Operation stencilFail, Operation depthFail, Operation bothPass)
{
	glStencilOp(EnumToValue(stencilFail), EnumToValue(depthFail), EnumToValue(bothPass));
}
//=============================================================================
void gpu::SetBlendingFunction(BlendFactor sourceFactor, BlendFactor destinationFactor)
{
	glBlendFunc(EnumToValue(sourceFactor), EnumToValue(destinationFactor));
}
//=============================================================================
void gpu::SetBlendingEquation(BlendEquation equation)
{
	glBlendEquation(EnumToValue(equation));
}
//=============================================================================
void gpu::SetCullFace(CullFace cullFace)
{
	glCullFace(EnumToValue(cullFace));
}
//=============================================================================
void gpu::SetDepthWriting(bool enable)
{
	glDepthMask(enable);
}
//=============================================================================
void gpu::SetColorWriting(bool enableRed, bool enableGreen, bool enableBlue, bool enableAlpha)
{
	glColorMask(enableRed, enableGreen, enableBlue, enableAlpha);
}
//=============================================================================
void gpu::SetViewport(float x, float y, float width, float height)
{
	glViewport(x, y, width, height);
}
////=============================================================================
//void gpu::DrawElements(PrimitiveMode primitiveMode, uint32_t indexCount)
//{
//	glDrawElements(EnumToValue(primitiveMode), indexCount, GL_UNSIGNED_INT, nullptr);
//}
////=============================================================================
//void gpu::DrawElementsInstanced(PrimitiveMode primitiveMode, uint32_t indexCount, uint32_t instances)
//{
//	glDrawElementsInstanced(EnumToValue(primitiveMode), indexCount, GL_UNSIGNED_INT, nullptr, instances);
//}
////=============================================================================
//void gpu::DrawArrays(PrimitiveMode primitiveMode, uint32_t vertexCount)
//{
//	glDrawArrays(EnumToValue(primitiveMode), 0, vertexCount);
//}
////=============================================================================
//void gpu::DrawArraysInstanced(PrimitiveMode primitiveMode, uint32_t vertexCount, uint32_t instances)
//{
//	glDrawArraysInstanced(EnumToValue(primitiveMode), 0, vertexCount, instances);
//}
//=============================================================================