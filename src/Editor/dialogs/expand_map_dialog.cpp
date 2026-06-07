#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::ExpandMapDialog::ExpandMapDialog(int& outAxis, int& outAmount)
	: _outAxis(outAxis)
	, _outAmount(outAmount)
{}
//=============================================================================
bool ed::ExpandMapDialog::Draw()
{
	if (_confirmed)
		return false;

	ImGui::OpenPopup("Expand Map");
	ImGui::SetNextWindowSize(ImVec2(300, 0));

	if (!ImGui::BeginPopupModal("Expand Map", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	static const char* AXIS_NAMES[] = { "+Z", "-Z", "+X", "-X", "+Y", "-Y" };
	ImGui::Combo("Axis", &_axis, AXIS_NAMES, 6);

	ImGui::InputInt("Amount", &_amount);
	if (_amount < 1)
		_amount = 1;

	if (ImGui::Button("Expand"))
	{
		_outAxis = _axis;
		_outAmount = _amount;
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
