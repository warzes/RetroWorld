#pragma once

#include "gpu_system.h"

namespace gpu
{
	bool Init();
	void Close();

	bool BeginFrame();
	void EndFrame();

} // namespace gpu