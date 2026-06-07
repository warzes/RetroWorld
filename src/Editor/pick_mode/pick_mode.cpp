#include "stdafx.h"
#include "pick_mode.hpp"
#include "../assets.hpp"
#include "../editor_app.hpp"
#include <imgui/imgui.h>
#include <regex>
#include <algorithm>
//=============================================================================
ed::PickMode::PickMode(int maxSelectionCount, std::string_view fileExtension)
	: _fileExtension(fileExtension)
	, _maxSelectionCount(maxSelectionCount)
{}
//=============================================================================
void ed::PickMode::rebuildFrames()
{
	_frames.clear();
	_selectedCount = 0;

	for (const auto& path : _foundFiles)
	{
		// Filter by search term
		if (_searchFilter[0] != '\0')
		{
			std::string label = path.stem().string();
			std::string lowerLabel = label;
			std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			std::string lowerFilter = _searchFilter;
			std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

			if (lowerLabel.find(lowerFilter) == std::string::npos)
				continue;
		}

		Frame frame;
		frame.filePath = path;
		frame.label = path.stem().string();
		frame.texture = gpu::texture::TexturePtr();
		_frames.push_back(std::move(frame));
	}
}
//=============================================================================
void ed::PickMode::OnEnter()
{
	_foundFiles.clear();

	if (_rootDir.empty() || !std::filesystem::is_directory(_rootDir))
		return;

	// Recursive scan with regex filtering
	const auto& settings = EditorApp::Get().GetSettings();
	std::regex hiddenFileRegex;
	try
	{
		hiddenFileRegex = std::regex(settings.assetHideRegex,
			std::regex_constants::icase | std::regex_constants::ECMAScript);
	}
	catch (...) {}

	for (const auto& entry : std::filesystem::recursive_directory_iterator(_rootDir))
	{
		if (entry.is_directory() || !entry.is_regular_file())
			continue;

		std::string ext = entry.path().extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		if (ext != _fileExtension)
			continue;

		if (std::regex_match(entry.path().string(), hiddenFileRegex))
			continue;

		_foundFiles.insert(entry.path());
	}

	rebuildFrames();
}
//=============================================================================
void ed::PickMode::Update()
{}
//=============================================================================
void ed::PickMode::Draw()
{
	const float WINDOW_UPPER_MARGIN = 24.0f;
	bool open = true;
	ImGui::SetNextWindowSize(ImVec2(
		static_cast<float>(window::GetWidth()),
		static_cast<float>(window::GetHeight()) - WINDOW_UPPER_MARGIN));
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(ImVec2(
		center.x, center.y + WINDOW_UPPER_MARGIN),
		ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::Begin("##Pick Mode View", &open,
		ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove))
	{
		float windowWidth = ImGui::GetWindowWidth();

		if (ImGui::InputText("Search", _searchFilter, SEARCH_BUFFER_SIZE))
			rebuildFrames();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 8.0f));
		ImGui::Separator();
		ImGui::PopStyleVar();

		int numCols = glm::max(
			static_cast<int>(windowWidth / (FRAME_SIZE * 1.5f)), 1);

		if (ImGui::BeginTable("##Frames", numCols,
			ImGuiTableFlags_BordersOuter | ImGuiTableFlags_ScrollY))
		{
			for (size_t i = 0; i < _frames.size(); ++i)
			{
				ImGui::TableNextColumn();
				auto& frame = _frames[i];

				ImGui::PushID(static_cast<int>(i));

				// Selected color
				ImVec4 color = ImVec4(1, 1, 1, 1);
				if (IsFrameSelected(frame.filePath))
					color = ImVec4(1, 1, 0, 1);

				ImGui::PushStyleColor(ImGuiCol_Button, color);

				// Lazy-load texture on first visible frame
				if (!frame.texture || !gpu::texture::IsValid(frame.texture))
					frame.texture = GetFrameTexture(frame.filePath);

				if (frame.texture && gpu::texture::IsValid(frame.texture))
				{
					uint32_t handle = gpu::texture::Handle(frame.texture);
					ImGui::ImageButton("##preview",
						static_cast<ImTextureID>(handle),
						ImVec2(ICON_SIZE, ICON_SIZE));
				}
				else
				{
					ImGui::Button("##preview", ImVec2(ICON_SIZE, ICON_SIZE));
				}

				if (ImGui::IsItemClicked())
					SelectFrame(frame);

				ImGui::PopStyleColor();

				ImGui::TextColored(color, "%s", frame.label.c_str());

				ImGui::PopID();
			}
			ImGui::EndTable();
		}
		ImGui::End();
	}
}
//=============================================================================
void ed::PickMode::OnExit()
{}
//=============================================================================
std::string ed::PickMode::GetSideLabel(const Frame& frame)
{
	return frame.label;
}
//=============================================================================
