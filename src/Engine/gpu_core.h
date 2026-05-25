#pragma once

namespace gpu
{
	constexpr inline uint64_t WHOLE_BUFFER = static_cast<uint64_t>(-1);

	enum class IndexType : uint32_t
	{
		UNSIGNED_BYTE,
		UNSIGNED_SHORT,
		UNSIGNED_INT,
	};

	enum class GlFormatClass
	{
		FLOAT,
		INT,
		LONG
	};

	enum class Format : uint32_t
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
	GLenum FormatToTypeGL(Format format);
	GLint FormatToSizeGL(Format format);
	GLboolean IsFormatNormalizedGL(Format format);
	GlFormatClass FormatToFormatClass(Format format);

	enum class RenderingCapability : uint8_t
	{
		Blend,
		CullFace,
		DepthTest,
		Dither,
		PolygonOffsetFill,
		SampleAlphaToCoverage,
		SampleCoverage,
		ScissorTest,
		StencilTest,
		Multisample
	};

	enum class RasterizationMode : uint8_t
	{
		Point,
		Line,
		Fill
	};

	enum class ComparisonFunc : uint8_t
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

	enum class Operation : uint8_t
	{
		Zero,
		Keep,
		Replace,
		Increment,
		IncrementWrap,
		Decrement,
		DecrementWrap,
		Invert
	};

	// Face Culling Type
	enum class CullFace : uint8_t
	{
		Front,
		Back,
		FrontAndBack
	};

	// Blend Equation Type
	enum class BlendEquation : uint8_t
	{
		Add,
		Subtract,
		ReverseSubtract,
		Min,
		Max
	};

	// Blend Mode Type
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

	// Uniform Type
	enum class UniformType : uint8_t
	{
		Float,
		Int,
		Vec2,
		Vec3,
		Vec4,
		Mat4,
		Sampler2D,
		Sampler2DShadow,
		USampler2D,
		SamplerCube,
		Image2D_RGBA32F,
		Block
	};

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

	enum class TextureTarget : uint8_t
	{
		Texture1D,
		Texture2D,
		Texture3D,
		TextureCube
	};

	enum class TextureFormat : uint8_t
	{
		RGBA8,
		RGB8,
		RG8,
		R16UI,
		R32UI,
		R32F,
		RGBA16F,
		RGBA32F,
		A8,
		R8,

		Depth8,
		Depth16,
		Depth24,
		Depth32F,
		Depth24_Stencil8,
		Depth32F_Stencil8,
		Stencil8,
	};

	enum class TextureWrapMode : uint8_t
	{
		Repeat,
		MirroredRepeat,
		ClampToEdge,
		ClampToBorder
	};

	enum class TextureFilter : uint8_t
	{
		Nearest,
		Linear,
		NearestMipmapNearest,
		LinearMipmapNearest,
		NearestMipmapLinear,
		LinearMipmapLinear
	};

	struct Metrics final
	{
		unsigned long drawCalls = 0; //< Mesh draw call.
		unsigned long quadCalls = 0; // Full screen quad.
		unsigned long stateChanges = 0; // State changes.
		unsigned long textureBindings = 0; // Number of texture bindings.
		unsigned long framebufferBindings = 0; // Number of framebuffer bindings.
		unsigned long bufferBindings = 0; // Number of data buffer bindings.
		unsigned long vertexBindings = 0; // Number of vertex array bindings.
		unsigned long programBindings = 0; // Number of shade program bindings.
		unsigned long clearAndBlits = 0; // Framebuffer clearing and blitting operations.
		unsigned long uploads = 0; // Data upload to the GPU.
		unsigned long downloads = 0; // Data download from the GPU.
		unsigned long uniforms = 0; // Uniform update.
	};
	inline Metrics MetricsCurrent;
	inline Metrics MetricsPrevious;
} // namespace gpu