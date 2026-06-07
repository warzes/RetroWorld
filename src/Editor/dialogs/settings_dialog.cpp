#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::SettingsDialog::SettingsDialog(size_t& undoMax, float& mouseSens, bool& cullFaces, std::string& hideRegex)
	: _undoMax(undoMax)
	, _mouseSens(mouseSens)
	, _cullFaces(cullFaces)
	, _hideRegex(hideRegex)
{
	strcpy_s(_hideRegexBuf, hideRegex.c_str());
}
//=============================================================================
bool ed::SettingsDialog::Draw()
{
	if (_confirmed)
		return false;

	ImGui::OpenPopup("Settings");
	ImGui::SetNextWindowSize(ImVec2(350, 0));

	if (!ImGui::BeginPopupModal("Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	int undoVal = static_cast<int>(_undoMax);
	ImGui::InputInt("Undo max", &undoVal);
	if (undoVal < 1)
		undoVal = 1;
	_undoMax = static_cast<size_t>(undoVal);

	ImGui::SliderFloat("Mouse sensitivity", &_mouseSens, 0.001f, 0.01f);

	ImGui::Checkbox("Cull faces", &_cullFaces);

	ImGui::InputText("Asset hide regex", _hideRegexBuf, sizeof(_hideRegexBuf));

	if (ImGui::Button("Save"))
	{
		_hideRegex = _hideRegexBuf;
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
