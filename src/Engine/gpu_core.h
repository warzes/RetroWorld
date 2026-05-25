#pragma once

namespace gpu
{
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