#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::NewMapDialog::NewMapDialog(int& outWidth, int& outHeight, int& outLength)
	: _width(100)
	, _height(5)
	, _length(100)
	, _outWidth(outWidth)
	, _outHeight(outHeight)
	, _outLength(outLength)
{}
//=============================================================================
bool ed::NewMapDialog::Draw()
{
	if (_confirmed)
		return false;

	ImGui::OpenPopup("New Map");
	ImGui::SetNextWindowSize(ImVec2(300, 0));

	if (!ImGui::BeginPopupModal("New Map", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	ImGui::InputInt("Width", &_width);
	ImGui::InputInt("Height", &_height);
	ImGui::InputInt("Length", &_length);

	if (ImGui::Button("Create"))
	{
		_outWidth = _width;
		_outHeight = _height;
		_outLength = _length;
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
