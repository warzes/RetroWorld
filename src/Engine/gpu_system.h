#pragma once

#include "gpu_core.h"

namespace gpu
{
	void SetClearColor(float red, float green, float blue, float alpha = 1.0f);
	void Clear(bool colorBuffer, bool depthBuffer, bool stencilBuffer = false);

	void SetCapability(RenderingCapability capability, bool value);
	bool GetCapability(RenderingCapability capability);

	void SetRasterizationLinesWidth(float width);
	void SetRasterizationMode(RasterizationMode rasterizationMode);

	void SetStencilAlgorithm(ComparisonFunc algorithm, int32_t reference, uint32_t mask);
	void SetDepthAlgorithm(ComparisonFunc algorithm);
	void SetStencilMask(uint32_t mask);
	void SetStencilOperations(Operation stencilFail, Operation depthFail, Operation bothPass);

	void SetBlendingFunction(BlendFactor sourceFactor, BlendFactor destinationFactor);
	void SetBlendingEquation(BlendEquation equation);

	void SetCullFace(CullFace cullFace);

	void SetDepthWriting(bool enable);
	void SetColorWriting(bool enableRed, bool enableGreen, bool enableBlue, bool enableAlpha);

	void SetViewport(float x, float y, float width, float height);

	void DrawElements(PrimitiveMode primitiveMode, uint32_t indexCount);
	void DrawElementsInstanced(PrimitiveMode primitiveMode, uint32_t indexCount, uint32_t instances);
	void DrawArrays(PrimitiveMode primitiveMode, uint32_t vertexCount);
	void DrawArraysInstanced(PrimitiveMode primitiveMode, uint32_t vertexCount, uint32_t instances);
} // namespace gpu