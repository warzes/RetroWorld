#include "stdafx.h"
#include "ent.hpp"
#include "handle.hpp"
//=============================================================================
ed::Ent::Ent(float radius_)
	: active(true)
	, radius(radius_)
{}
//=============================================================================
ed::Ent ed::EntGrid::GetEnt(int i, int j, int k) const
{
	assert(HasEnt(i, j, k));
	return GetCel(i, j, k);
}
//=============================================================================
ed::EntGrid ed::EntGrid::Subsection(int i, int j, int k, int w, int h, int l) const
{
	assert(i >= 0 && j >= 0 && k >= 0);
	assert(i + w <= static_cast<int>(_width) && j + h <= static_cast<int>(_height) && k + l <= static_cast<int>(_length));

	EntGrid newGrid(static_cast<size_t>(w), static_cast<size_t>(h), static_cast<size_t>(l));
	SubsectionCopy(i, j, k, w, h, l, newGrid);
	return newGrid;
}
//=============================================================================
std::vector<ed::Ent> ed::EntGrid::GetEntList() const
{
	std::vector<Ent> result;
	result.reserve(_grid.size());
	for (size_t i = 0; i < _grid.size(); ++i)
		if (_grid[i].active)
			result.push_back(_grid[i]);
	return result;
}
//=============================================================================
ed::EntGrid::EntGrid()
	: EntGrid(0, 0, 0)
{}
//=============================================================================
ed::EntGrid::EntGrid(size_t width, size_t height, size_t length)
	: Grid<Ent>(width, height, length, ENT_SPACING_DEFAULT, Ent())
{}
//=============================================================================
void ed::to_json(nlohmann::json& j, const Ent& ent)
{
	j["radius"] = ent.radius;
	j["color"] = nlohmann::json::array({ ent.color.x, ent.color.y, ent.color.z });
	j["position"] = nlohmann::json::array({
		ent.lastRenderedPosition.x, ent.lastRenderedPosition.y, ent.lastRenderedPosition.z });
	j["angles"] = nlohmann::json::array({ ent.pitch, ent.yaw, 0.0f });
	j["display"] = static_cast<int>(ent.display);
	if (ent.model) j["model"] = ent.model->GetPath().generic_string();
	if (ent.texture) j["texture"] = ent.texture->GetPath().generic_string();
	j["properties"] = ent.properties;
}
//=============================================================================
void ed::from_json(const nlohmann::json& j, Ent& ent)
{
	ent.active = true;
	ent.radius = j.at("radius").get<float>();
	auto col = j.at("color");
	ent.color = glm::vec3(col[0].get<float>(), col[1].get<float>(), col[2].get<float>());
	auto pos = j.at("position");
	ent.lastRenderedPosition = glm::vec3(pos[0].get<float>(), pos[1].get<float>(), pos[2].get<float>());
	auto ang = j.at("angles");
	ent.pitch = ang[0].get<int>();
	ent.yaw = ang[1].get<int>();
	ent.properties = j.at("properties").get<std::map<std::string, std::string>>();

	if (j.contains("display"))
		ent.display = static_cast<Ent::DisplayMode>(j.at("display").get<int>());
	else
		ent.display = Ent::DisplayMode::SPHERE;
}
//=============================================================================