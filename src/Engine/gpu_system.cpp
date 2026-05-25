#include "stdafx.h"
#include "_gpu_system.h"
#include "gpu_program.h"
#include "app_window.h"
#include "core_log.h"
//=============================================================================
inline GLenum EnumToValue(gpu::RenderingCapability c) noexcept
{
	switch (c)
	{
	case gpu::RenderingCapability::Blend:                 return GL_BLEND;
	case gpu::RenderingCapability::CullFace:              return GL_CULL_FACE;
	case gpu::RenderingCapability::DepthTest:             return GL_DEPTH_TEST;
	case gpu::RenderingCapability::Dither:                return GL_DITHER;
	case gpu::RenderingCapability::PolygonOffsetFill:     return GL_POLYGON_OFFSET_FILL;
	case gpu::RenderingCapability::SampleAlphaToCoverage: return GL_SAMPLE_ALPHA_TO_COVERAGE;
	case gpu::RenderingCapability::SampleCoverage:        return GL_SAMPLE_COVERAGE;
	case gpu::RenderingCapability::ScissorTest:           return GL_SCISSOR_TEST;
	case gpu::RenderingCapability::StencilTest:           return GL_STENCIL_TEST;
	case gpu::RenderingCapability::Multisample:           return GL_MULTISAMPLE;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::RasterizationMode mode) noexcept
{
	switch (mode) {
	case gpu::RasterizationMode::Point: return GL_POINT;
	case gpu::RasterizationMode::Line:  return GL_LINE;
	case gpu::RasterizationMode::Fill:  return GL_FILL;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::ComparisonFunc func) noexcept
{
	switch (func) {
	case gpu::ComparisonFunc::Never:        return GL_NEVER;
	case gpu::ComparisonFunc::Less:         return GL_LESS;
	case gpu::ComparisonFunc::Equal:        return GL_EQUAL;
	case gpu::ComparisonFunc::LessEqual:    return GL_LEQUAL;
	case gpu::ComparisonFunc::Greater:      return GL_GREATER;
	case gpu::ComparisonFunc::NotEqual:     return GL_NOTEQUAL;
	case gpu::ComparisonFunc::GreaterEqual: return GL_GEQUAL;
	case gpu::ComparisonFunc::Always:       return GL_ALWAYS;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::Operation op) noexcept
{
	switch (op)
	{
	case gpu::Operation::Zero:          return GL_ZERO;
	case gpu::Operation::Keep:          return GL_KEEP;
	case gpu::Operation::Replace:       return GL_REPLACE;
	case gpu::Operation::Increment:     return GL_INCR;
	case gpu::Operation::IncrementWrap: return GL_INCR_WRAP;
	case gpu::Operation::Decrement:     return GL_DECR;
	case gpu::Operation::DecrementWrap: return GL_DECR_WRAP;
	case gpu::Operation::Invert:        return GL_INVERT;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::BlendFactor factor) noexcept
{
	switch (factor) {
	case gpu::BlendFactor::Zero:                  return GL_ZERO;
	case gpu::BlendFactor::One:                   return GL_ONE;
	case gpu::BlendFactor::SrcColor:              return GL_SRC_COLOR;
	case gpu::BlendFactor::OneMinusSrcColor:      return GL_ONE_MINUS_SRC_COLOR;
	case gpu::BlendFactor::DstColor:              return GL_DST_COLOR;
	case gpu::BlendFactor::OneMinusDstColor:      return GL_ONE_MINUS_DST_COLOR;
	case gpu::BlendFactor::SrcAlpha:              return GL_SRC_ALPHA;
	case gpu::BlendFactor::OneMinusSrcAlpha:      return GL_ONE_MINUS_SRC_ALPHA;
	case gpu::BlendFactor::DstAlpha:              return GL_DST_ALPHA;
	case gpu::BlendFactor::OneMinusDstAlpha:      return GL_ONE_MINUS_DST_ALPHA;
	case gpu::BlendFactor::ConstantColor:         return GL_CONSTANT_COLOR;
	case gpu::BlendFactor::OneMinusConstantColor: return GL_ONE_MINUS_CONSTANT_COLOR;
	case gpu::BlendFactor::ConstantAlpha:         return GL_CONSTANT_ALPHA;
	case gpu::BlendFactor::OneMinusConstantAlpha: return GL_ONE_MINUS_CONSTANT_ALPHA;
	case gpu::BlendFactor::SrcAlphaSaturate:      return GL_SRC_ALPHA_SATURATE;
	case gpu::BlendFactor::Src1Color:             return GL_SRC1_COLOR;
	case gpu::BlendFactor::OneMinusSrc1Color:     return GL_ONE_MINUS_SRC1_COLOR;
	case gpu::BlendFactor::Src1Alpha:             return GL_SRC1_ALPHA;
	case gpu::BlendFactor::OneMinusSrc1Alpha:     return GL_ONE_MINUS_SRC1_ALPHA;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::BlendEquation eq) noexcept
{
	switch (eq) {
	case gpu::BlendEquation::Add:             return GL_FUNC_ADD;
	case gpu::BlendEquation::Subtract:        return GL_FUNC_SUBTRACT;
	case gpu::BlendEquation::ReverseSubtract: return GL_FUNC_REVERSE_SUBTRACT;
	case gpu::BlendEquation::Min:                 return GL_MIN;
	case gpu::BlendEquation::Max:                 return GL_MAX;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::CullFace cull) noexcept
{
	switch (cull) {
	case gpu::CullFace::Front:        return GL_FRONT;
	case gpu::CullFace::Back:         return GL_BACK;
	case gpu::CullFace::FrontAndBack: return GL_FRONT_AND_BACK;
	default: std::unreachable();
	}
}
//=============================================================================
inline GLenum EnumToValue(gpu::PrimitiveMode mode) noexcept
{
	switch (mode) {
	case gpu::PrimitiveMode::Points:                 return GL_POINTS;
	case gpu::PrimitiveMode::Lines:                  return GL_LINES;
	case gpu::PrimitiveMode::LineLoop:               return GL_LINE_LOOP;
	case gpu::PrimitiveMode::LineStrip:              return GL_LINE_STRIP;
	case gpu::PrimitiveMode::Triangles:              return GL_TRIANGLES;
	case gpu::PrimitiveMode::TriangleStrip:          return GL_TRIANGLE_STRIP;
	case gpu::PrimitiveMode::TriangleFan:            return GL_TRIANGLE_FAN;
	case gpu::PrimitiveMode::LinesAdjacency:         return GL_LINES_ADJACENCY;
	case gpu::PrimitiveMode::LineStripAdjacency:     return GL_LINE_STRIP_ADJACENCY;
	case gpu::PrimitiveMode::TrianglesAdjacency:     return GL_TRIANGLES_ADJACENCY;
	case gpu::PrimitiveMode::TriangleStripAdjacency: return GL_TRIANGLE_STRIP_ADJACENCY;
	default: std::unreachable();
	}
}
//=============================================================================
namespace
{
}
//=============================================================================
bool gpu::Init()
{
	//windowSize.width = window::GetWidth();
	//windowSize.height = window::GetHeight();

	const char* vendor = (const char*)glGetString(GL_VENDOR);
	const char* renderer = (const char*)glGetString(GL_RENDERER);
	const char* version = (const char*)glGetString(GL_VERSION);
	core::Print("Vendor: " + std::string(vendor));
	core::Print("Renderer: " + std::string(renderer));
	core::Print("OpenGL version supported: " + std::string(version));

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	MetricsCurrent = MetricsPrevious = { 0 };

	gpu::program::BindShaderProgram(nullptr);

	return true;
}
//=============================================================================
void gpu::Close()
{
}
//=============================================================================
bool gpu::BeginFrame()
{
	if (window::GetWindowMinimized())
		return false;

	//if (windowSize.width != window::GetWidth() || windowSize.height != window::GetHeight())
	//{
	//	windowSize.width = window::GetWidth();
	//	windowSize.height = window::GetHeight();
	//}

	MetricsPrevious = MetricsCurrent;
	MetricsCurrent = { 0 };

	return true;
}
//=============================================================================
void gpu::EndFrame()
{

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
void gpu::SetStencilAlgorithm(ComparisonFunc algorithm, int32_t reference, uint32_t mask)
{
	glStencilFunc(EnumToValue(algorithm), reference, mask);
}
//=============================================================================
void gpu::SetDepthAlgorithm(ComparisonFunc algorithm)
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
//=============================================================================
void gpu::DrawElements(PrimitiveMode primitiveMode, uint32_t indexCount)
{
	glDrawElements(EnumToValue(primitiveMode), indexCount, GL_UNSIGNED_INT, nullptr);
}
//=============================================================================
void gpu::DrawElementsInstanced(PrimitiveMode primitiveMode, uint32_t indexCount, uint32_t instances)
{
	glDrawElementsInstanced(EnumToValue(primitiveMode), indexCount, GL_UNSIGNED_INT, nullptr, instances);
}
//=============================================================================
void gpu::DrawArrays(PrimitiveMode primitiveMode, uint32_t vertexCount)
{
	glDrawArrays(EnumToValue(primitiveMode), 0, vertexCount);
}
//=============================================================================
void gpu::DrawArraysInstanced(PrimitiveMode primitiveMode, uint32_t vertexCount, uint32_t instances)
{
	glDrawArraysInstanced(EnumToValue(primitiveMode), 0, vertexCount, instances);
}
//=============================================================================