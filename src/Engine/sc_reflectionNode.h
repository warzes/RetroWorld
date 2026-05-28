#pragma once

#include "sc_node.h"

namespace scene
{
	class ReflectionProbeNode final : public SceneNode
	{
	public:
		explicit ReflectionProbeNode(std::string name);

		// 6 view matrices for each cubemap face (+X, -X, +Y, -Y, +Z, -Z)
		std::array<glm::mat4, 6> GetCaptureMatrices() const;

		float     radius = 50.0f;
		uint32_t  cubemapHandle = 0;
		bool      isDirty = true;
	};
} //namespace scene