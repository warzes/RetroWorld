#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::AboutDialog::AboutDialog()
{}
//=============================================================================
bool ed::AboutDialog::Draw()
{
	ImGui::OpenPopup("About");
	ImGui::SetNextWindowSize(ImVec2(300, 0));

	if (!ImGui::BeginPopupModal("About", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		return true;

	ImGui::Text("Total Editor 3 Port");
	ImGui::Text("RetroWorld Engine");
	ImGui::Separator();
	ImGui::Text("Version 1.0.0");
	ImGui::Text("Build: %s %s", __DATE__, __TIME__);

	ImGui::Dummy(ImVec2(0, 10));

	if (ImGui::Button("Close"))
	{
		ImGui::CloseCurrentPopup();
		return false;
	}

	ImGui::EndPopup();
	return true;
}
