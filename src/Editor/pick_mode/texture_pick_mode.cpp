#include "stdafx.h"
#include "pick_mode.hpp"
#include "../assets.hpp"
#include "../editor_app.hpp"
//=============================================================================
ed::TexturePickMode::TexturePickMode()
	: PickMode(TEXTURES_PER_TILE, ".png")
{
	_rootDir = EditorApp::Get().GetTexturesDir();
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
	// Cycle: if slot 0 is taken, shift to slot 1 so new selection goes to slot 0
	if (_selectedTextures[0] && _selectedTextures[1])
	{
		_selectedTextures[0] = tex; // Replace slot 0
	}
	else if (_selectedTextures[0])
	{
		_selectedTextures[1] = _selectedTextures[0];
		_selectedTextures[0] = tex;
	}
	else
	{
		_selectedTextures[0] = tex;
	}
}
//=============================================================================
bool ed::TexturePickMode::IsFrameSelected(const std::filesystem::path& filePath)
{
	for (const auto& tex : _selectedTextures)
		if (tex && tex->GetPath() == filePath) return true;
	return false;
}
//=============================================================================