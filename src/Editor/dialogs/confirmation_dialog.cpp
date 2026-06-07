#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::ConfirmationDialog::ConfirmationDialog(const std::string& message, std::function<void(bool)> callback)
	: _message(message)
	, _callback(std::move(callback))
{}
//=============================================================================
bool ed::ConfirmationDialog::Draw()
{
	ImGui::OpenPopup("Confirm");
	ImGui::SetNextWindowSize(ImVec2(300, 0));

	if (!ImGui::BeginPopupModal("Confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	ImGui::TextWrapped("%s", _message.c_str());

	ImGui::Dummy(ImVec2(0, 10));

	if (ImGui::Button("Yes"))
	{
		if (_callback)
			_callback(true);
		ImGui::CloseCurrentPopup();
		return false;
	}

	ImGui::SameLine();

	if (ImGui::Button("No"))
	{
		if (_callback)
			_callback(false);
		ImGui::CloseCurrentPopup();
		return false;
	}

	ImGui::EndPopup();
	return true;
}
