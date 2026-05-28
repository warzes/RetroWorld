#include "stdafx.h"
#include "sc_lightNode.h"
//=============================================================================
scene::LightNode::LightNode(std::string name)
	: SceneNode(std::move(name), NodeType::Light)
{}
//=============================================================================