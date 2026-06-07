#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::InstructionsDialog::InstructionsDialog()
{}
//=============================================================================
bool ed::InstructionsDialog::Draw()
{
	ImGui::OpenPopup("Instructions");
	ImGui::SetNextWindowSize(ImVec2(500, 400));

	if (!ImGui::BeginPopupModal("Instructions", nullptr, ImGuiWindowFlags_NoResize))
		return true;

	ImGui::TextWrapped(
		"Welcome to Total Editor 3 Port!\n"
		"\n"
		"Camera Controls:\n"
		"  - Hold Right Mouse Button to look around\n"
		"  - WASD to move the camera\n"
		"  - Q/E to move up/down\n"
		"  - Shift to move faster\n"
		"\n"
		"Editing:\n"
		"  - Select brush shape with keys 1-7\n"
		"  - Left click to place blocks\n"
		"  - Hold Ctrl + Left click to remove blocks\n"
		"  - Hold Alt to select multiple blocks\n"
		"\n"
		"File Operations:\n"
		"  - Ctrl+N: New map\n"
		"  - Ctrl+O: Open map\n"
		"  - Ctrl+S: Save map\n"
		"  - Ctrl+E: Export map\n"
		"\n"
		"Tips:\n"
		"  - Use the Expand Map dialog to grow your map\n"
		"  - Configure asset paths in Settings\n"
		"  - Check the Shortcuts dialog for all keybindings\n"
	);

	ImGui::Dummy(ImVec2(0, 10));

	if (ImGui::Button("Close"))
	{
		ImGui::CloseCurrentPopup();
		return false;
	}

	ImGui::EndPopup();
	return true;
}
