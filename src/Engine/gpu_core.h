#pragma once

#include "core_flags.h"
#include "core_hash.h"
#include "core_baseTypes.h"

#ifndef SE_DEFAULT_CLIP_DEPTH_RANGE_ZERO_TO_ONE
#	define SE_DEFAULT_CLIP_DEPTH_RANGE_NEGATIVE_ONE_TO_ONE
#endif

namespace gpu
{
	enum class ImageType : uint8_t
	{
		Texture1D,
		Texture2D,
		Texture3D,
		Texture1DArray,
		Texture2DArray,
		TextureCubemap,
		TextureCubemapArray,
		Texture2DMultisample,
		Texture2DMultisampleArray
	};

	enum class ComponentSwizzle : uint8_t
	{
		ZERO,
		ONE,
		R,
		G,
		B,
		A
	};

	enum class Format : uint8_t
	{
		UNDEFINED,

		// Color formats
		R8_UNORM,
		R8_SNORM,
		R16_UNORM,
		R16_SNORM,
		R8G8_UNORM,
		R8G8_SNORM,
		R16G16_UNORM,
		R16G16_SNORM,
		R3G3B2_UNORM,
		R4G4B4_UNORM,
		R5G5B5_UNORM,
		R8G8B8_UNORM,
		R8G8B8_SNORM,
		R10G10B10_UNORM,
		R12G12B12_UNORM,
		R16G16B16_SNORM,
		R2G2B2A2_UNORM,
		R4G4B4A4_UNORM,
		R5G5B5A1_UNORM,
		R8G8B8A8_UNORM,
		R8G8B8A8_SNORM,
		R10G10B10A2_UNORM,
		R10G10B10A2_UINT,
		R12G12B12A12_UNORM,
		R16G16B16A16_UNORM,
		R16G16B16A16_SNORM,
		R8G8B8_SRGB,
		R8G8B8A8_SRGB,
		R16_FLOAT,
		R16G16_FLOAT,
		R16G16B16_FLOAT,
		R16G16B16A16_FLOAT,
		R32_FLOAT,
		R32G32_FLOAT,
		R32G32B32_FLOAT,
		R32G32B32A32_FLOAT,
		R11G11B10_FLOAT,
		R9G9B9_E5,
		R8_SINT,
		R8_UINT,
		R16_SINT,
		R16_UINT,
		R32_SINT,
		R32_UINT,
		R8G8_SINT,
		R8G8_UINT,
		R16G16_SINT,
		R16G16_UINT,
		R32G32_SINT,
		R32G32_UINT,
		R8G8B8_SINT,
		R8G8B8_UINT,
		R16G16B16_SINT,
		R16G16B16_UINT,
		R32G32B32_SINT,
		R32G32B32_UINT,
		R8G8B8A8_SINT,
		R8G8B8A8_UINT,
		R16G16B16A16_SINT,
		R16G16B16A16_UINT,
		R32G32B32A32_SINT,
		R32G32B32A32_UINT,

		// Depth & stencil formats
		D32_FLOAT,
		D32_UNORM,
		D24_UNORM,
		D16_UNORM,
		D32_FLOAT_S8_UINT,
		D24_UNORM_S8_UINT,
		S8_UINT,

		// Compressed formats
		// DXT
		BC1_RGB_UNORM,
		BC1_RGB_SRGB,
		BC1_RGBA_UNORM,
		BC1_RGBA_SRGB,
		BC2_RGBA_UNORM,
		BC2_RGBA_SRGB,
		BC3_RGBA_UNORM,
		BC3_RGBA_SRGB,
		// RGTC
		BC4_R_UNORM,
		BC4_R_SNORM,
		BC5_RG_UNORM,
		BC5_RG_SNORM,
		// BPTC
		BC6H_RGB_UFLOAT,
		BC6H_RGB_SFLOAT,
		BC7_RGBA_UNORM,
		BC7_RGBA_SRGB,

		// TODO: 64-bits-per-component formats
	};

	// multisampling and anisotropy
	enum class SampleCount : uint8_t
	{
		Samples1 = 1,
		Samples2 = 2,
		Samples4 = 4,
		Samples8 = 8,
		Samples16 = 16,
		Samples32 = 32,
	};

	enum class UploadFormat : uint8_t
	{
		UNDEFINED,
		R,
		RG,
		RGB,
		BGR,
		RGBA,
		BGRA,
		R_INTEGER,
		RG_INTEGER,
		RGB_INTEGER,
		BGR_INTEGER,
		RGBA_INTEGER,
		BGRA_INTEGER,
		DEPTH_COMPONENT,
		STENCIL_INDEX,
		DEPTH_STENCIL,

		// @brief For CopyTextureToBuffer and CopyBufferToTexture
		INFER_FORMAT,
	};

	enum class UploadType : uint8_t
	{
		UNDEFINED,
		UBYTE,
		SBYTE,
		USHORT,
		SSHORT,
		UINT,
		SINT,
		FLOAT,
		UBYTE_3_3_2,
		UBYTE_2_3_3_REV,
		USHORT_5_6_5,
		USHORT_5_6_5_REV,
		USHORT_4_4_4_4,
		USHORT_4_4_4_4_REV,
		USHORT_5_5_5_1,
		USHORT_1_5_5_5_REV,
		UINT_8_8_8_8,
		UINT_8_8_8_8_REV,
		UINT_10_10_10_2,
		UINT_2_10_10_10_REV,

		// For CopyTextureToBuffer and CopyBufferToTexture
		INFER_TYPE,
	};

	enum class Filter : uint8_t
	{
		None,
		Nearest,
		Linear,
		NearestMipmapNearest,
		LinearMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapLinear
	};

	enum class AddressMode : uint8_t
	{
		Repeat,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder,
		MirrorClampToEdge
	};

	enum class BorderColor : uint8_t
	{
		FloatTransparentBlack,
		IntTransparentBlack,
		FloatOpaqueBlack,
		IntOpaqueBlack,
		FloatOpaqueWhite,
		IntOpaqueWhite,
	};

	enum class AspectMaskBit : uint32_t
	{
		COLOR_BUFFER_BIT = 1 << 0,
		DEPTH_BUFFER_BIT = 1 << 1,
		STENCIL_BUFFER_BIT = 1 << 2,
	};
	SE_DECLARE_FLAG_TYPE(AspectMask, AspectMaskBit, uint32_t);

	enum class PrimitiveTopology : uint8_t
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
		TriangleFan,

		// available only in pipelines with tessellation shaders
		PatchList,
	};

	enum class PolygonMode : uint8_t
	{
		Point,
		Line,
		Fill
	};

	enum class CullMode : uint8_t
	{
		None,
		Front,
		Back,
		FrontAndBack
	};

	enum class FrontFace : uint8_t
	{
		ClockWise,
		CounterClockWise,
	};

	enum class CompareOp : uint8_t
	{
		Never,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always
	};

	enum class LogicOp : uint8_t
	{
		Clear,
		Set,
		Copy,
		CopyInverted,
		NoOp,
		Invert,
		And,
		Nand,
		Or,
		Nor,
		Xor,
		Equivalent,
		AndReverse,
		OrReverse,
		AndInverted,
		OrInverted,
	};

	enum class BlendFactor : uint8_t
	{
		Zero,
		One,
		SrcColor,
		OneMinusSrcColor,
		DstColor,
		OneMinusDstColor,
		SrcAlpha,
		OneMinusSrcAlpha,
		DstAlpha,
		OneMinusDstAlpha,

		ConstantColor,
		OneMinusConstantColor,
		ConstantAlpha,
		OneMinusConstantAlpha,
		SrcAlphaSaturate,
		Src1Color,
		OneMinusSrc1Color,
		Src1Alpha,
		OneMinusSrc1Alpha
	};

	enum class BlendOp : uint8_t
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max
	};

	enum class ColorComponentFlag : uint32_t
	{
		NONE,
		R_BIT = 0b0001,
		G_BIT = 0b0010,
		B_BIT = 0b0100,
		A_BIT = 0b1000,
		RGBA_BITS = 0b1111,
	};
	SE_DECLARE_FLAG_TYPE(ColorComponentFlags, ColorComponentFlag, uint32_t);

	enum class IndexType : uint8_t
	{
		UnsignedByte,
		UnsignedShort,
		UnsignedInt,
	};

	enum class MemoryBarrierBit : uint32_t
	{
		NONE = 0,
		VERTEX_BUFFER_BIT = 1 << 0,  // GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT
		INDEX_BUFFER_BIT = 1 << 1,  // GL_ELEMENT_ARRAY_BARRIER_BIT
		UNIFORM_BUFFER_BIT = 1 << 2,  // GL_UNIFORM_BARRIER_BIT
		TEXTURE_FETCH_BIT = 1 << 3,  // GL_TEXTURE_FETCH_BARRIER_BIT
		IMAGE_ACCESS_BIT = 1 << 4,  // GL_SHADER_IMAGE_ACCESS_BARRIER_BIT
		COMMAND_BUFFER_BIT = 1 << 5,  // GL_COMMAND_BARRIER_BIT
		TEXTURE_UPDATE_BIT = 1 << 6,  // GL_TEXTURE_UPDATE_BARRIER_BIT
		BUFFER_UPDATE_BIT = 1 << 7,  // GL_BUFFER_UPDATE_BARRIER_BIT
		MAPPED_BUFFER_BIT = 1 << 8,  // GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT
		FRAMEBUFFER_BIT = 1 << 9,  // GL_FRAMEBUFFER_BARRIER_BIT
		SHADER_STORAGE_BIT = 1 << 10, // GL_SHADER_STORAGE_BARRIER_BIT
		QUERY_COUNTER_BIT = 1 << 11, // GL_QUERY_BUFFER_BARRIER_BIT
		ALL_BITS = static_cast<uint32_t>(-1),
		// TODO: add more bits as necessary
	};
	SE_DECLARE_FLAG_TYPE(MemoryBarrierBits, MemoryBarrierBit, uint32_t);

	enum class StencilOp : uint8_t
	{
		Keep,
		Zero,
		Replace,
		IncrementClamp,
		IncrementWrap,
		DecrementClamp,
		DecrementWrap,
		Invert
	};

	enum class ClipDepthRange : uint8_t
	{
		NegativeOneToOne, // OpenGL default
		ZeroToOne         // D3D and Vulkan
	};

	struct DrawIndirectCommand final
	{
		uint32_t vertexCount;
		uint32_t instanceCount;
		uint32_t firstVertex;
		uint32_t firstInstance;
	};

	struct DrawIndexedIndirectCommand final
	{
		uint32_t indexCount;
		uint32_t instanceCount;
		uint32_t firstIndex;
		int32_t vertexOffset;
		uint32_t firstInstance;
	};

	struct DispatchIndirectCommand final
	{
		uint32_t groupCountX;
		uint32_t groupCountY;
		uint32_t groupCountZ;
	};

	struct Viewport final
	{
		bool operator==(const Viewport&) const noexcept = default;

		core::Rect2D drawRect = {}; // glViewport
		float minDepth = 0.0f;      // glDepthRangef
		float maxDepth = 1.0f;      // glDepthRangef
		ClipDepthRange depthRange = // glClipControl
#ifdef SE_DEFAULT_CLIP_DEPTH_RANGE_NEGATIVE_ONE_TO_ONE
			ClipDepthRange::NegativeOneToOne;
#else
			ClipDepthRange::ZeroToOne;
#endif
	};

	struct Scissor final
	{
		bool operator==(const Scissor&) const = default;

		glm::vec2 position = { 0.0f, 0.0f };
		glm::vec2 size = { 0.0f, 0.0f };
	};

	struct Metrics final
	{
		unsigned long drawCalls = 0;           //< Mesh draw call.
		unsigned long quadCalls = 0;           // Full screen quad.
		unsigned long stateChanges = 0;        // State changes.
		unsigned long textureBindings = 0;     // Number of texture bindings.
		unsigned long framebufferBindings = 0; // Number of framebuffer bindings.
		unsigned long bufferBindings = 0;      // Number of data buffer bindings.
		unsigned long vertexBindings = 0;      // Number of vertex array bindings.
		unsigned long programBindings = 0;     // Number of shade program bindings.
		unsigned long clearAndBlits = 0;       // Framebuffer clearing and blitting operations.
		unsigned long uploads = 0;             // Data upload to the GPU.
		unsigned long downloads = 0;           // Data download from the GPU.
		unsigned long uniforms = 0;            // Uniform update.
	};
	inline Metrics MetricsCurrent;
	inline Metrics MetricsPrevious;

	class ScopedDebugMarker final
	{
	public:
		ScopedDebugMarker(const char* message);
		ScopedDebugMarker(const ScopedDebugMarker&) = delete;
		~ScopedDebugMarker();
	};

} // namespace gpu