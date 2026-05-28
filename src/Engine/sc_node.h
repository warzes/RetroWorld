#pragma once

#include "math_transform.h"

namespace scene
{
	enum class NodeType : uint8_t
	{
		Base,
		Model,
		Light,
		Camera,
		ReflectionProbe
	};

	class SceneNode
	{
	public:
		explicit SceneNode(std::string name, NodeType type = NodeType::Base);

		// Virtual-like destructor for safe polymorphic deletion through unique_ptr<SceneNode>
		virtual ~SceneNode() = default;

		// Template: create child of type T, forward args, set parent, return reference
		template<typename T, typename... Args> requires std::derived_from<T, SceneNode>
		T& AddChild(Args&&... args)
		{
			auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
			ptr->parent = this;
			auto& ref = *ptr;
			children.push_back(std::move(ptr));
			return ref;
		}

		// Remove first child matching name; returns true if found and removed
		bool RemoveChild(std::string_view childName);

		// Find first child by name (depth-first); returns nullptr if not found
		SceneNode* FindChild(std::string_view childName);

		// Get cached world transform from last traversal
		const glm::mat4& GetWorldTransform() const { return cachedWorldMatrix; }

		// Pre-order traversal; fn receives each node and its world matrix
		void Traverse(const std::function<void(SceneNode&, const glm::mat4&)>& fn);

		std::string                       name;
		math::Transform                   transform;
		NodeType                          type = NodeType::Base;

		SceneNode*                        parent = nullptr;
		std::vector<std::unique_ptr<SceneNode>> children;

		// Cached world matrix (updated during traverse)
		glm::mat4                         cachedWorldMatrix = glm::mat4(1.0f);

		bool                              visible = true;
		bool                              isStatic = false;
	};
} // namespace scene