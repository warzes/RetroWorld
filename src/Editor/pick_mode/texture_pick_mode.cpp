#include "stdafx.h"
#include "pick_mode.hpp"
#include "../assets.hpp"
#include "../editor_app.hpp"
#include "../place_mode/place_mode.hpp"
//=============================================================================
ed::TexturePickMode::TexturePickMode()
	: PickMode(TEXTURES_PER_TILE, ".png")
{}
//=============================================================================
void ed::TexturePickMode::OnEnter()
{
	_rootDir = EditorApp::Get().GetTexturesDir();
	PickMode::OnEnter();
}
//=============================================================================
gpu::texture::TexturePtr ed::TexturePickMode::GetFrameTexture(const std::filesystem::path& filePath)
{
	auto handle = Assets::GetTexture(filePath);
	return handle ? handle->GetTexture() : gpu::texture::TexturePtr();
}
//=============================================================================
void ed::TexturePickMode::SelectFrame(const Frame& frame)
{
	auto tex = Assets::GetTexture(frame.filePath);

	if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		_selectedTextures[0] = tex;
	else if (ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		_selectedTextures[1] = tex;

	auto& app = EditorApp::Get();
	app.ChangeEditorMode(EditorApp::Mode::PLACE_TILE);
	app.GetPlaceMode().SetCursorTextures(_selectedTextures);
}
//=============================================================================
bool ed::TexturePickMode::IsFrameSelected(const std::filesystem::path& filePath)
{
	for (const auto& tex : _selectedTextures)
		if (tex && tex->GetPath() == filePath) return true;
	return false;
}
//=============================================================================
std::string ed::TexturePickMode::GetSideLabel(const Frame& frame)
{
	bool primary = _selectedTextures[0] && frame.filePath == _selectedTextures[0]->GetPath();
	bool secondary = _selectedTextures[1] && frame.filePath == _selectedTextures[1]->GetPath();
	std::string label;
	if (primary) label += "(Primary)";
	if (secondary) label += (primary ? "\n" : "") + std::string("(Secondary)");
	return label;
}
//=============================================================================