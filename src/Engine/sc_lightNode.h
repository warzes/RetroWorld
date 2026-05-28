#pragma once

#include "sc_node.h"

namespace scene
{
	struct ShadowSettings final
	{
		bool    enabled = true;
		int     resolution = 1024;
		float   bias = 0.005f;
		float   normalBias = 0.02f;
		float   orthoSize = 100.0f;  // half-extent for directional light ortho projection
		float   cascadeDistance[4] = { 10.0f, 30.0f, 60.0f, 100.0f };
	};

	struct LightData final
	{
		glm::vec4  positionOrDirection = glm::vec4(0.0f); // w=0 dir, 1 point, 2 spot
		glm::vec3  color = glm::vec3(1.0f);
		float      intensity = 1.0f;
		glm::vec3  attenuation = glm::vec3(1.0f, 0.0f, 0.0f);
		float      radius = 10.0f;
		glm::vec3  spotDirection = glm::vec3(0.0f, -1.0f, 0.0f);
		float      innerCutoff = 0.0f; // cos(innerAngle)
		float      outerCutoff = 0.0f; // cos(outerAngle)
		int        type = 0;    // 0=dir, 1=point, 2=spot
		int        castShadow = 0;
		float      shadowBias = 0.005f;
		glm::mat4  lightSpaceMatrix = glm::mat4(1.0f);
	};

	class LightNode final : public SceneNode
	{
	public:
		explicit LightNode(std::string name);

		enum class LightType : uint8_t { Directional, Point, Spot };

		LightType lightType = LightType::Directional;
		glm::vec3 color = glm::vec3(1.0f);
		float     intensity = 1.0f;

		// Point / Spot
		float     radius = 10.0f;
		glm::vec3 attenuation = glm::vec3(1.0f, 0.09f, 0.032f);

		// Spot
		float     innerAngle = glm::radians(12.5f);
		float     outerAngle = glm::radians(17.5f);

		// Directional / Spot direction in local space
		glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

		bool          castShadow = true;
		ShadowSettings shadowSettings;
	};
} //namespace scene