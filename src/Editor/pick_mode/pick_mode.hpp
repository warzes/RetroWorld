#pragma once

#include <set>
#include <vector>
#include <string>
#include <filesystem>
#include <memory>
#include <array>
#include <cstring>
#include <unordered_map>
#include "../editor_app.hpp"
#include "../handle.hpp"
#include "../tile.hpp"
#include <gpu_texture.h>
#include <gpu_framebuffer.h>

namespace ed
{
	class PickMode : public EditorApp::ModeImpl
	{
	public:
		static constexpr int SEARCH_BUFFER_SIZE = 256;
		static constexpr int FRAME_SIZE = 196;
		static constexpr int ICON_SIZE = 64;

		struct Frame
		{
			std::filesystem::path filePath;
			std::string label;
			gpu::texture::TexturePtr texture;
		};

		PickMode(int maxSelectionCount, std::string_view fileExtension);
		~PickMode() override = default;
		void OnEnter() override;
		void Update() override;
		void Draw() override;
		void OnExit() override;

	protected:
		virtual gpu::texture::TexturePtr GetFrameTexture(const std::filesystem::path& filePath) = 0;
		virtual void SelectFrame(const Frame& frame) = 0;
		virtual bool IsFrameSelected(const std::filesystem::path& filePath) = 0;
		virtual std::string GetSideLabel(const Frame& frame);

		void rebuildFrames();

		std::set<std::filesystem::path> _foundFiles;
		std::vector<Frame> _frames;
		std::filesystem::path _rootDir;
		std::string _fileExtension;
		int _maxSelectionCount;
		int _selectedCount = 0;

	private:
		char _searchFilter[SEARCH_BUFFER_SIZE]{};
	};

	class TexturePickMode final : public PickMode
	{
	public:
		using TexSelection = std::array<std::shared_ptr<TexHandle>, TEXTURES_PER_TILE>;

		explicit TexturePickMode();
		~TexturePickMode() override = default;

		TexSelection GetPickedTextures() const { return _selectedTextures; }
		void SetPickedTextures(TexSelection newTextures) { _selectedTextures = newTextures; }

	protected:
		gpu::texture::TexturePtr GetFrameTexture(const std::filesystem::path& filePath) override;
		void SelectFrame(const Frame& frame) override;
		bool IsFrameSelected(const std::filesystem::path& filePath) override;

	private:
		TexSelection _selectedTextures;
	};

	class ShapePickMode final : public PickMode
	{
	public:
		ShapePickMode();
		~ShapePickMode() override = default;

		void OnEnter() override;
		void Update() override;

		std::shared_ptr<ModelHandle> GetPickedShape() const { return _selectedShape; }
		void SetPickedShape(std::shared_ptr<ModelHandle> newModel) { _selectedShape = newModel; }

	protected:
		gpu::texture::TexturePtr GetFrameTexture(const std::filesystem::path& filePath) override;
		void SelectFrame(const Frame& frame) override;
		bool IsFrameSelected(const std::filesystem::path& filePath) override;

	private:
		struct PreviewEntry
		{
			gpu::texture::TexturePtr colorTex;
			gpu::fbo::FramebufferPtr fbo;
		};

		void createPreviewFBO(const std::filesystem::path& path);
		void renderPreview(const std::filesystem::path& path, float angleRad);

		std::shared_ptr<ModelHandle> _selectedShape;
		std::unordered_map<std::filesystem::path, PreviewEntry> _previews;
		float _previewRotation = 0.0f;
		size_t _nextCreateIndex = 0;
	};
} // namespace ed