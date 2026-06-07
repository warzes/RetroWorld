#include "stdafx.h"
#include "dialogs.hpp"
#include <imgui/imgui.h>
#include <vector>
//=============================================================================
namespace fs = std::filesystem;
//=============================================================================
ed::FileDialog::FileDialog(std::string& outPath, const std::string& title,
	const std::string& extension, bool save)
	: _title(title)
	, _extension(extension)
	, _save(save)
	, _outPath(outPath)
{
	_baseDir = fs::current_path();
	if (!save)
	{
		_currentPath = _baseDir.string();
	}
	else
	{
		_currentPath = _baseDir.string();
	}
}
//=============================================================================
void ed::FileDialog::SetCurrentPath(const fs::path& path)
{
	_baseDir = path;
	_currentPath = path.string();
}
//=============================================================================
bool ed::FileDialog::Draw()
{
	if (_confirmed)
		return false;

	ImGui::OpenPopup(_title.c_str());
	ImGui::SetNextWindowSize(ImVec2(500, 400));

	if (!ImGui::BeginPopupModal(_title.c_str(), nullptr, ImGuiWindowFlags_NoResize))
		return true;

	// Current path display
	ImGui::Text("Path: %s", _currentPath.c_str());
	ImGui::Separator();

	// File list
	ImGui::BeginChild("##files", ImVec2(0, -60), true);

	std::vector<fs::directory_entry> entries;
	if (fs::exists(_currentPath))
	{
		for (const auto& entry : fs::directory_iterator(_currentPath))
		{
			entries.push_back(entry);
		}
	}

	// Sort: directories first, then files
	std::sort(entries.begin(), entries.end(),
		[](const fs::directory_entry& a, const fs::directory_entry& b)
		{
			if (a.is_directory() != b.is_directory())
				return a.is_directory() > b.is_directory();
			return a.path().filename() < b.path().filename();
		});

	int idx = 0;
	for (const auto& entry : entries)
	{
		auto filename = entry.path().filename().string();
		bool isDir = entry.is_directory();

		// Filter files by extension
		if (!isDir)
		{
			auto ext = entry.path().extension().string();
			if (!_extension.empty() && ext != _extension)
				continue;
		}

		std::string label;
		if (isDir)
			label = "[Dir] " + filename;
		else
			label = filename;

		bool selected = (_fileName == filename);
		if (ImGui::Selectable(label.c_str(), &selected))
		{
			strcpy_s(_fileName, filename.c_str());
		}

		// Double-click to enter directory
		if (isDir && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
		{
			_currentPath = entry.path().string();
			_fileName[0] = '\0';
		}

		idx++;
	}

	ImGui::EndChild();
	ImGui::Separator();

	// File name input
	if (_save)
	{
		ImGui::InputText("File name", _fileName, sizeof(_fileName));
	}
	else if (strlen(_fileName) > 0)
	{
		ImGui::Text("Selected: %s", _fileName);
	}

	// Buttons
	if (ImGui::Button(_save ? "Save" : "Open"))
	{
		auto path = fs::path(_currentPath) / _fileName;
		_outPath = path.string();
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
