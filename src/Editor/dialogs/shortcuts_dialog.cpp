#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
//=============================================================================
ed::ShortcutsDialog::ShortcutsDialog()
{}
//=============================================================================
bool ed::ShortcutsDialog::Draw()
{
	ImGui::OpenPopup("Shortcuts");
	ImGui::SetNextWindowSize(ImVec2(400, 350));

	if (!ImGui::BeginPopupModal("Shortcuts", nullptr, ImGuiWindowFlags_NoResize))
		return true;

	if (ImGui::BeginTable("##shortcuts", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("Key");
		ImGui::TableSetupColumn("Action");
		ImGui::TableHeadersRow();

		auto row = [](const char* key, const char* action)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%s", key);
			ImGui::TableNextColumn();
			ImGui::Text("%s", action);
		};

		row("W", "Move camera forward");
		row("S", "Move camera backward");
		row("A", "Move camera left");
		row("D", "Move camera right");
		row("Q", "Move camera up");
		row("E", "Move camera down");
		row("Shift", "Speed up camera");
		row("Ctrl+Z", "Undo");
		row("Ctrl+Y", "Redo");
		row("Ctrl+S", "Save map");
		row("Ctrl+O", "Open map");
		row("Ctrl+N", "New map");
		row("Ctrl+E", "Export map");
		row("Delete", "Delete selected");
		row("F", "Focus on selection");
		row("1-7", "Select brush shape");

		ImGui::EndTable();
	}

	ImGui::Dummy(ImVec2(0, 10));

	if (ImGui::Button("Close"))
	{
		ImGui::CloseCurrentPopup();
		return false;
	}

	ImGui::EndPopup();
	return true;
}
