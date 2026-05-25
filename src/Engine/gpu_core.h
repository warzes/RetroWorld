#pragma once

namespace gpu
{
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

	// Primitive Type
	enum class PrimitiveMode : uint8_t
	{
		Points,
		Lines,
		LineLoop,
		LineStrip,
		Triangles,
		TriangleStrip,
		TriangleFan,
		LinesAdjacency,
		LineStripAdjacency,
		TrianglesAdjacency,
		TriangleStripAdjacency,
	};

	// Vertex Atribute Type
	enum class VertexAttribute : uint8_t
	{
		Float4,
		Float3,
		Float2,
		Float,
		UInt4,
		UInt3,
		UInt2,
		UInt,
		Byte4,
		Byte3,
		Byte2,
		Byte
	};

	enum class VertexAttribType : uint8_t // TODO: delete
	{
		Float,
		Uint8,
		Int,
	};

	// Buffer Type
	enum class BufferType : uint8_t
	{
		Vertex,
		Index,
		Frame,
		Uniform,
		UniformConstant,
		ShaderStorage,
		Sampler
	};

	// Buffer Usage Type
	enum class BufferUsage : uint8_t
	{
		Static,
		Dynamic,
		Stream
	};

	// Buffer Update Type
	enum class BufferUpdate : uint8_t
	{
		Recreate,
		SubData
	};

	enum class BufferMapAccess : uint8_t
	{
		Read,
		Write,
		ReadWrite
	};

	enum BufferMapRangeAccess : uint8_t
	{
		Read,
		Write,
		InvalidateRange,
		InvalidateBuffer,
		FlushExplicit,
		Unsynchronized
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