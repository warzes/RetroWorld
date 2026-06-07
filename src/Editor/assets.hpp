#pragma once

#include <memory>
#include <map>
#include <vector>
#include <string>
#include <filesystem>
#include <imgui/imgui.h>

#include "handle.hpp"

namespace ed
{
	class Assets final
	{
	public:
		static Assets& Get() noexcept
		{
			static Assets instance;
			return instance;
		}

		// Resource access
		static std::shared_ptr<TexHandle> GetTexture(const std::filesystem::path& path);
		static std::shared_ptr<ModelHandle> GetModel(const std::filesystem::path& path);

		// Pre-built assets
		static std::shared_ptr<gr::Mesh> GetEntSphere() noexcept;
		static std::shared_ptr<gr::Mesh> GetSpriteQuad() noexcept;

		// UI fonts
		static ImFont* GetUIFont(float size = 18.0f);
		static ImFont* GetCodeFont() noexcept;

		// Missing fallback assets
		static gpu::texture::TexturePtr GetMissingTexture();
		static std::shared_ptr<gr::Mesh> GetMissingModel();

		static void Init(const std::filesystem::path& texturesDir, const std::filesystem::path& shapesDir);
		static void ReloadTexturePaths(const std::filesystem::path& texturesDir);
		static void ReloadModelPaths(const std::filesystem::path& shapesDir);

		[[nodiscard]] const std::vector<std::filesystem::path>& GetTexturePaths() const noexcept { return _texturePaths; }
		[[nodiscard]] const std::vector<std::filesystem::path>& GetModelPaths() const noexcept { return _modelPaths; }

	private:
		Assets() = default;
		~Assets() = default;
		Assets(const Assets&) = delete;
		Assets& operator=(const Assets&) = delete;

		void discoverTextures(const std::filesystem::path& dir);
		void discoverModels(const std::filesystem::path& dir);

		std::map<std::filesystem::path, std::weak_ptr<TexHandle>> _textureCache;
		std::map<std::filesystem::path, std::weak_ptr<ModelHandle>> _modelCache;

		std::vector<std::filesystem::path> _texturePaths;
		std::vector<std::filesystem::path> _modelPaths;

		std::shared_ptr<gr::Mesh> _entSphere;
		std::shared_ptr<gr::Mesh> _spriteQuad;

		ImFont* _uiFontLarge = nullptr;
		ImFont* _codeFont = nullptr;

		bool _initialized = false;
	};
} // namespace ed