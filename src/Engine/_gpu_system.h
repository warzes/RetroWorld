#pragma once

#include "gpu_system.h"
#include "gpu_deviceInfo.h"

namespace gpu
{
	bool Init();
	void Close();

	bool BeginFrame();
	void EndFrame();

	void QueryGlDeviceProperties(DeviceProperties& properties);

} // namespace gpu