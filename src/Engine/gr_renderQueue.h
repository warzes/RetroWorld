#pragma once

namespace scene
{
	class ModelNode;
} //namespace scene

namespace gr
{
	struct RenderItem final
	{
		scene::ModelNode* node = nullptr; // non-owning
		glm::mat4         worldTransform = glm::mat4(1.0f);
		float             distanceToCamera = 0.0f;
		uintptr_t         materialId = 0; // address of Material object for sorting
		bool              isInstanced = false;
	};

	class RenderQueue final
	{
	public:
		// Sort opaque: materialId ascending, then front-to-back
		// Sort transparent: back-to-front
		void Sort();

		// Clear both queues
		void Clear();

		// Add item to appropriate queue based on material transparency
		void Submit(const RenderItem& item, bool isTransparent);

		std::vector<RenderItem> opaqueItems;
		std::vector<RenderItem> transparentItems;
	};
} //namespace gr