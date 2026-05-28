#pragma once

#include "gpu_core.h"

namespace gpu
{
	struct InputAssemblyState final
	{
		PrimitiveTopology topology  = PrimitiveTopology::TriangleList;
		bool primitiveRestartEnable = false;
	};

	struct TessellationState final
	{
		uint32_t patchControlPoints{ 0 }; // glPatchParameteri(GL_PATCH_VERTICES, ...)
	};

	struct RasterizationState final
	{
		bool depthClampEnable         = false;
		PolygonMode polygonMode       = PolygonMode::Fill;
		CullMode cullMode             = CullMode::Back;
		FrontFace frontFace           = FrontFace::CounterClockWise;
		bool depthBiasEnable          = false;
		float depthBiasConstantFactor = 0.0f;
		float depthBiasSlopeFactor    = 0.0f;
		float lineWidth               = 1.0f; // glLineWidth
		float pointSize               = 1.0f; // glPointSize
	};

	struct MultisampleState final
	{
		bool sampleShadingEnable   = false;      // glEnable(GL_SAMPLE_SHADING)
		float minSampleShading     = 1.0f;       // glMinSampleShading
		uint32_t sampleMask        = 0xFFFFFFFF; // glSampleMaski
		bool alphaToCoverageEnable = false;      // glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE)
		bool alphaToOneEnable      = false;      // glEnable(GL_SAMPLE_ALPHA_TO_ONE)
	};

	struct DepthState final
	{
		bool depthTestEnable     = false;           // gl{Enable, Disable}(GL_DEPTH_TEST)
		bool depthWriteEnable    = false;           // glDepthMask(depthWriteEnable)
		CompareOp depthCompareOp = CompareOp::Less; // glDepthFunc
	};

	struct StencilOpState final
	{
		bool operator==(const StencilOpState&) const noexcept = default;

		StencilOp passOp      = StencilOp::Keep;   // glStencilOp (dppass)
		StencilOp failOp      = StencilOp::Keep;   // glStencilOp (sfail)
		StencilOp depthFailOp = StencilOp::Keep;   // glStencilOp (dpfail)
		CompareOp compareOp   = CompareOp::Always; // glStencilFunc (func)
		uint32_t compareMask  = 0;                 // glStencilFunc (mask)
		uint32_t writeMask    = 0;                 // glStencilMask
		uint32_t reference    = 0;                 // glStencilFunc (ref)
	};

	struct StencilState final
	{
		bool stencilTestEnable = false;
		StencilOpState front = {};
		StencilOpState back = {};
	};

	struct ColorBlendAttachmentState final // glBlendFuncSeparatei + glBlendEquationSeparatei
	{
		bool operator==(const ColorBlendAttachmentState&) const noexcept = default;

		bool blendEnable                = false;              // if false, blend factor = one?
		BlendFactor srcColorBlendFactor = BlendFactor::One;   // srcRGB
		BlendFactor dstColorBlendFactor = BlendFactor::Zero;  // dstRGB
		BlendOp colorBlendOp            = BlendOp::Add;       // modeRGB
		BlendFactor srcAlphaBlendFactor = BlendFactor::One;   // srcAlpha
		BlendFactor dstAlphaBlendFactor = BlendFactor::Zero;  // dstAlpha
		BlendOp alphaBlendOp            = BlendOp::Add;       // modeAlpha
		ColorComponentFlags colorWriteMask = ColorComponentFlag::RGBA_BITS; // glColorMaski
	};

	struct ColorBlendState final
	{
		bool logicOpEnable = false;       // gl{Enable, Disable}(GL_COLOR_LOGIC_OP)
		LogicOp logicOp = LogicOp::Copy;  // glLogicOp(logicOp)
		std::vector<ColorBlendAttachmentState> attachments = {}; // glBlendFuncSeparatei + glBlendEquationSeparatei
		float blendConstants[4] = { 0, 0, 0, 0 }; // glBlendColor
	};

} // namespace gpu