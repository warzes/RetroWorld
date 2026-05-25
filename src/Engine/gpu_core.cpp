#include "stdafx.h"
#include "gpu_core.h"
//=============================================================================
GLenum gpu::FormatToTypeGL(Format format)
{
	switch (format)
	{
	case Format::R8_UNORM:
	case Format::R8G8_UNORM:
	case Format::R8G8B8_UNORM:
	case Format::R8G8B8A8_UNORM:
	case Format::R8_UINT:
	case Format::R8G8_UINT:
	case Format::R8G8B8_UINT:
	case Format::R8G8B8A8_UINT:
	case Format::R8G8B8A8_SRGB:
	case Format::R8G8B8_SRGB:
		return GL_UNSIGNED_BYTE;
	case Format::R8_SNORM:
	case Format::R8G8_SNORM:
	case Format::R8G8B8_SNORM:
	case Format::R8G8B8A8_SNORM:
	case Format::R8_SINT:
	case Format::R8G8_SINT:
	case Format::R8G8B8_SINT:
	case Format::R8G8B8A8_SINT:
		return GL_BYTE;
	case Format::R16_UNORM:
	case Format::R16G16_UNORM:
	case Format::R16G16B16A16_UNORM:
	case Format::R16_UINT:
	case Format::R16G16_UINT:
	case Format::R16G16B16_UINT:
	case Format::R16G16B16A16_UINT:
		return GL_UNSIGNED_SHORT;
	case Format::R16_SNORM:
	case Format::R16G16_SNORM:
	case Format::R16G16B16_SNORM:
	case Format::R16G16B16A16_SNORM:
	case Format::R16_SINT:
	case Format::R16G16_SINT:
	case Format::R16G16B16_SINT:
	case Format::R16G16B16A16_SINT:
		return GL_SHORT;
	case Format::R16_FLOAT:
	case Format::R16G16_FLOAT:
	case Format::R16G16B16_FLOAT:
	case Format::R16G16B16A16_FLOAT:
		return GL_HALF_FLOAT;
	case Format::R32_FLOAT:
	case Format::R32G32_FLOAT:
	case Format::R32G32B32_FLOAT:
	case Format::R32G32B32A32_FLOAT:
		return GL_FLOAT;
	case Format::R32_SINT:
	case Format::R32G32_SINT:
	case Format::R32G32B32_SINT:
	case Format::R32G32B32A32_SINT:
		return GL_INT;
	case Format::R32_UINT:
	case Format::R32G32_UINT:
	case Format::R32G32B32_UINT:
	case Format::R32G32B32A32_UINT:
		return GL_UNSIGNED_INT;
	default: std::unreachable();
	}
}
//=============================================================================
GLint gpu::FormatToSizeGL(Format format)
{
	switch (format)
	{
	case Format::R8_UNORM:
	case Format::R8_SNORM:
	case Format::R16_UNORM:
	case Format::R16_SNORM:
	case Format::R16_FLOAT:
	case Format::R32_FLOAT:
	case Format::R8_SINT:
	case Format::R16_SINT:
	case Format::R32_SINT:
	case Format::R8_UINT:
	case Format::R16_UINT:
	case Format::R32_UINT:
		return 1;
	case Format::R8G8_UNORM:
	case Format::R8G8_SNORM:
	case Format::R16G16_FLOAT:
	case Format::R16G16_UNORM:
	case Format::R16G16_SNORM:
	case Format::R32G32_FLOAT:
	case Format::R8G8_SINT:
	case Format::R16G16_SINT:
	case Format::R32G32_SINT:
	case Format::R8G8_UINT:
	case Format::R16G16_UINT:
	case Format::R32G32_UINT:
		return 2;
	case Format::R8G8B8_UNORM:
	case Format::R8G8B8_SNORM:
	case Format::R16G16B16_SNORM:
	case Format::R16G16B16_FLOAT:
	case Format::R32G32B32_FLOAT:
	case Format::R8G8B8_SINT:
	case Format::R16G16B16_SINT:
	case Format::R32G32B32_SINT:
	case Format::R8G8B8_UINT:
	case Format::R16G16B16_UINT:
	case Format::R32G32B32_UINT:
		return 3;
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_SNORM:
	case Format::R16G16B16A16_UNORM:
	case Format::R16G16B16A16_SNORM:
	case Format::R16G16B16A16_FLOAT:
	case Format::R32G32B32A32_FLOAT:
	case Format::R8G8B8A8_SINT:
	case Format::R16G16B16A16_SINT:
	case Format::R32G32B32A32_SINT:
	case Format::R10G10B10A2_UINT:
	case Format::R8G8B8A8_UINT:
	case Format::R16G16B16A16_UINT:
	case Format::R32G32B32A32_UINT:
		return 4;
	default: std::unreachable();
	}
}
//=============================================================================
GLboolean gpu::IsFormatNormalizedGL(Format format)
{
	switch (format)
	{
	case Format::R8_UNORM:
	case Format::R8_SNORM:
	case Format::R16_UNORM:
	case Format::R16_SNORM:
	case Format::R8G8_UNORM:
	case Format::R8G8_SNORM:
	case Format::R16G16_UNORM:
	case Format::R16G16_SNORM:
	case Format::R8G8B8_UNORM:
	case Format::R8G8B8_SNORM:
	case Format::R16G16B16_SNORM:
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_SNORM:
	case Format::R16G16B16A16_UNORM:
	case Format::R16G16B16A16_SNORM:
		return GL_TRUE;
	case Format::R16_FLOAT:
	case Format::R32_FLOAT:
	case Format::R8_SINT:
	case Format::R16_SINT:
	case Format::R32_SINT:
	case Format::R8_UINT:
	case Format::R16_UINT:
	case Format::R32_UINT:
	case Format::R16G16_FLOAT:
	case Format::R32G32_FLOAT:
	case Format::R8G8_SINT:
	case Format::R16G16_SINT:
	case Format::R32G32_SINT:
	case Format::R8G8_UINT:
	case Format::R16G16_UINT:
	case Format::R32G32_UINT:
	case Format::R16G16B16_FLOAT:
	case Format::R32G32B32_FLOAT:
	case Format::R8G8B8_SINT:
	case Format::R16G16B16_SINT:
	case Format::R32G32B32_SINT:
	case Format::R8G8B8_UINT:
	case Format::R16G16B16_UINT:
	case Format::R32G32B32_UINT:
	case Format::R16G16B16A16_FLOAT:
	case Format::R32G32B32A32_FLOAT:
	case Format::R8G8B8A8_SINT:
	case Format::R16G16B16A16_SINT:
	case Format::R32G32B32A32_SINT:
	case Format::R10G10B10A2_UINT:
	case Format::R8G8B8A8_UINT:
	case Format::R16G16B16A16_UINT:
	case Format::R32G32B32A32_UINT:
		return GL_FALSE;
	default: std::unreachable();
	}
}
//=============================================================================
gpu::GlFormatClass gpu::FormatToFormatClass(Format format)
{
	switch (format)
	{
	case Format::R8_UNORM:
	case Format::R8_SNORM:
	case Format::R16_UNORM:
	case Format::R16_SNORM:
	case Format::R8G8_UNORM:
	case Format::R8G8_SNORM:
	case Format::R16G16_UNORM:
	case Format::R16G16_SNORM:
	case Format::R8G8B8_UNORM:
	case Format::R8G8B8_SNORM:
	case Format::R16G16B16_SNORM:
	case Format::R8G8B8A8_UNORM:
	case Format::R8G8B8A8_SNORM:
	case Format::R16G16B16A16_UNORM:
	case Format::R16G16B16A16_SNORM:
	case Format::R16_FLOAT:
	case Format::R16G16_FLOAT:
	case Format::R16G16B16_FLOAT:
	case Format::R16G16B16A16_FLOAT:
	case Format::R32_FLOAT:
	case Format::R32G32_FLOAT:
	case Format::R32G32B32_FLOAT:
	case Format::R32G32B32A32_FLOAT:
		return GlFormatClass::FLOAT;
	case Format::R8_SINT:
	case Format::R16_SINT:
	case Format::R32_SINT:
	case Format::R8G8_SINT:
	case Format::R16G16_SINT:
	case Format::R32G32_SINT:
	case Format::R8G8B8_SINT:
	case Format::R16G16B16_SINT:
	case Format::R32G32B32_SINT:
	case Format::R8G8B8A8_SINT:
	case Format::R16G16B16A16_SINT:
	case Format::R32G32B32A32_SINT:
	case Format::R10G10B10A2_UINT:
	case Format::R8_UINT:
	case Format::R16_UINT:
	case Format::R32_UINT:
	case Format::R8G8_UINT:
	case Format::R16G16_UINT:
	case Format::R32G32_UINT:
	case Format::R8G8B8_UINT:
	case Format::R16G16B16_UINT:
	case Format::R32G32B32_UINT:
	case Format::R8G8B8A8_UINT:
	case Format::R16G16B16A16_UINT:
	case Format::R32G32B32A32_UINT:
		return GlFormatClass::INT;
	default: std::unreachable(); return GlFormatClass::LONG;
	}
}
//=============================================================================