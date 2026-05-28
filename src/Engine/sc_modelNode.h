#pragma once

#include "sc_node.h"
#include "gr_mesh.h"
#include "gr_material.h"

namespace scene
{
	class ModelNode final : public SceneNode
	{
	public:
		explicit ModelNode(std::string name);

		void AddInstance(const glm::mat4& transform);
		void ClearInstances();

		std::shared_ptr<gr::Mesh>     mesh;
		std::shared_ptr<gr::Material> material;

		bool castShadow = true;
		bool receiveShadow = true;
		bool excludeFromReflections = false;

		// Manual instancing transforms (in addition to auto-instancing)
		std::vector<glm::mat4> instanceTransforms;
	};
} // namespace scene