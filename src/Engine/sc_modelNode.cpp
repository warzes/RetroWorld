#include "stdafx.h"
#include "sc_modelNode.h"
//=============================================================================
scene::ModelNode::ModelNode(std::string name)
	: SceneNode(std::move(name), NodeType::Model)
{}
//=============================================================================
void scene::ModelNode::AddInstance(const glm::mat4& transform)
{
	instanceTransforms.push_back(transform);
}
//=============================================================================
void scene::ModelNode::ClearInstances()
{
	instanceTransforms.clear();
}
//=============================================================================