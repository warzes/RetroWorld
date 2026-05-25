#pragma once

#include "gpu_core.h"

namespace gpu
{
	struct ContextState final
	{
		// Used for scope error checking
		bool isComputeActive = false;
		bool isRendering = false;

		// Used for error checking for indexed draws
		bool isIndexBufferBound = false;
		IndexType currentIndexType{};

	} inline context;
} // namespace gpu