#include "stdafx.h"
#include "sc_node.h"
//=============================================================================
scene::SceneNode::SceneNode(std::string name_, NodeType type_)
	: name(std::move(name_))
	, type(type_)
{}
//=============================================================================
bool scene::SceneNode::RemoveChild(std::string_view childName)
{
	auto it = std::ranges::find_if(children, [&](const auto& child) {
		return child->name == childName;
		});
	if (it != children.end())
	{
		children.erase(it);
		return true;
	}
	return false;
}
//=============================================================================
scene::SceneNode* scene::SceneNode::FindChild(std::string_view childName)
{
	// Check immediate children first
	for (const auto& child : children)
	{
		if (child->name == childName)
			return child.get();
	}
	// Depth-first recursion
	for (const auto& child : children)
	{
		auto* found = child->FindChild(childName);
		if (found) return found;
	}
	return nullptr;
}
//=============================================================================
void scene::SceneNode::Traverse(const std::function<void(SceneNode&, const glm::mat4&)>& fn)
{
	glm::mat4 parentWorld = parent ? parent->cachedWorldMatrix : glm::mat4(1.0f);
	cachedWorldMatrix = transform.GetWorldMatrix(parentWorld);
	fn(*this, cachedWorldMatrix);
	for (auto& child : children)
		child->Traverse(fn);
}
//=============================================================================