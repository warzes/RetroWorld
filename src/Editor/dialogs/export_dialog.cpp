#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::ExportDialog::ExportDialog(std::string& outPath, bool& separateGeometry)
	: _outPath(outPath)
	, _outSeparate(separateGeometry)
{
	strcpy_s(_path, outPath.c_str());
	_separate = separateGeometry;
}
//=============================================================================
bool ed::ExportDialog::Draw()
{
	if (_confirmed)
		return false;

	ImGui::OpenPopup("Export");
	ImGui::SetNextWindowSize(ImVec2(350, 0));

	if (!ImGui::BeginPopupModal("Export", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	ImGui::InputText("File path", _path, sizeof(_path));
	ImGui::Checkbox("Separate geometry", &_separate);

	if (ImGui::Button("Export"))
	{
		_outPath = _path;
		_outSeparate = _separate;
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
