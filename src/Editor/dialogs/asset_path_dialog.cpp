#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::AssetPathDialog::AssetPathDialog(std::string& outTexDir, std::string& outShapeDir)
	: _outTexDir(outTexDir)
	, _outShapeDir(outShapeDir)
{
	strcpy_s(_texDir, outTexDir.c_str());
	strcpy_s(_shapeDir, outShapeDir.c_str());
}
//=============================================================================
bool ed::AssetPathDialog::Draw()
{
	if (_confirmed)
		return false;

	ImGui::OpenPopup("Asset Paths");
	ImGui::SetNextWindowSize(ImVec2(400, 0));

	if (!ImGui::BeginPopupModal("Asset Paths", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	ImGui::InputText("Textures directory", _texDir, sizeof(_texDir));
	ImGui::InputText("Shapes directory", _shapeDir, sizeof(_shapeDir));

	if (ImGui::Button("Save"))
	{
		_outTexDir = _texDir;
		_outShapeDir = _shapeDir;
		_confirmed = true;
		ImGui::CloseCurrentPopup();
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		_confirmed = true;
		ImGui::CloseCurrentPopup();
	}

	ImGui::EndPopup();
	return true;
}
