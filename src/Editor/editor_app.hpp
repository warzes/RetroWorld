#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <glm/glm.hpp>

#include <gpu_program.h>
#include "editor_math.hpp"

namespace scene { class SceneNode; class SceneManager; class CameraNode; }
namespace ed { class MapMan; class MenuBar; class PlaceMode; class TexturePickMode; class ShapePickMode; class EntMode; }

namespace ed
{
	class EditorApp final
	{
	public:
		struct Settings
		{
			std::string texturesDir = "data/textures";
			std::string shapesDir = "data/assets/models/shapes";
			size_t undoMax = 64;
			float mouseSensitivity = 0.002f;
			bool exportSeparateGeometry = false;
			bool cullFaces = true;
			std::string exportFilePath;
			std::string defaultTexturePath;
			std::string defaultShapePath;
			glm::vec3 backgroundColor = glm::vec3(0.12f, 0.12f, 0.12f);
			std::string assetHideRegex;

			static void to_json(nlohmann::json& j, const Settings& s);
			static void from_json(const nlohmann::json& j, Settings& s);
		};

		enum class Mode : uint8_t
		{
			PLACE_TILE,
			PICK_TEXTURE,
			PICK_SHAPE,
			EDIT_ENT
		};

		class ModeImpl
		{
		public:
			virtual ~ModeImpl() = default;
			virtual void Update() = 0;
			virtual void Draw() = 0;
			virtual void OnEnter() = 0;
			virtual void OnExit() = 0;
		};

		static EditorApp& Get() noexcept
		{
			static EditorApp instance;
			return instance;
		}

		void ChangeEditorMode(Mode newMode);

		// Settings accessors
		[[nodiscard]] float GetMouseSensitivity() const noexcept { return _settings.mouseSensitivity; }
		[[nodiscard]] size_t GetUndoMax() const noexcept { return _settings.undoMax; }
		[[nodiscard]] const std::string& GetTexturesDir() const noexcept { return _settings.texturesDir; }
		[[nodiscard]] const std::string& GetShapesDir() const noexcept { return _settings.shapesDir; }
		[[nodiscard]] const std::string& GetDefaultTexturePath() const noexcept { return _settings.defaultTexturePath; }
		[[nodiscard]] const std::string& GetDefaultShapePath() const noexcept { return _settings.defaultShapePath; }
		[[nodiscard]] bool IsCullingEnabled() const noexcept { return _settings.cullFaces; }
		[[nodiscard]] glm::vec3 GetBackgroundColor() const noexcept { return _settings.backgroundColor; }

		[[nodiscard]] bool IsPreviewing() const noexcept { return _previewDraw; }
		void SetPreviewing(bool p) noexcept { _previewDraw = p; }
		void TogglePreviewing() noexcept { _previewDraw = !_previewDraw; }

		[[nodiscard]] const std::filesystem::path& GetLastSavedPath() const noexcept { return _lastSavedPath; }
		[[nodiscard]] bool DidSave() const noexcept { return _didSave; }
		[[nodiscard]] bool IsQuitting() const noexcept { return _quit; }
		void Quit() noexcept { _quit = true; }

		void DisplayStatusMessage(std::string message, float durationSeconds, int priority);

		void Update();
		void Init(int argc, char* argv[]);

		[[nodiscard]] MapMan& GetMapMan() noexcept { return *_mapMan; }
		[[nodiscard]] const MapMan& GetMapMan() const noexcept { return *_mapMan; }

		// Settings access
		[[nodiscard]] Settings& GetSettings() noexcept { return _settings; }

		// Scene manager & menu bar
		[[nodiscard]] scene::SceneManager& GetSceneManager() noexcept { return *_sceneManager; }
		[[nodiscard]] MenuBar& GetMenuBar() noexcept { return *_menuBar; }

		// Mode access
		[[nodiscard]] PlaceMode& GetPlaceMode() noexcept { return *_tilePlaceMode; }
		[[nodiscard]] TexturePickMode& GetTexPickMode() noexcept { return *_texPickMode; }
		[[nodiscard]] ShapePickMode& GetShapePickMode() noexcept { return *_shapePickMode; }
		[[nodiscard]] EntMode& GetEntMode() noexcept { return *_entMode; }
		[[nodiscard]] ModeImpl* GetEditorModePtr() noexcept { return _editorMode; }
		[[nodiscard]] Mode GetEditorMode() const noexcept { return _currentMode; }
		void ClearGameSystems();

		void ResetEditorCamera();
		void NewMap(int width, int height, int length);
		void ExpandMap(Direction axis, int amount);
		void ShrinkMap();
		void TryOpenMap(const std::filesystem::path& path);
		void TrySaveMap(const std::filesystem::path& path);
		void TryExportMap(const std::filesystem::path& path, bool separateGeometry);
		void SaveSettings();
		void LoadSettings();

		// Engine integration
		void SyncMapToSceneGraph();
		void InitGameSystems();

		// Access BlinnPhong shader for offscreen previews
		static gpu::program::ShaderProgramPtr GetBlinnPhongProgram();

	private:
		EditorApp();
		~EditorApp();
		EditorApp(const EditorApp&) = delete;
		EditorApp& operator=(const EditorApp&) = delete;

		Settings _settings;
		std::unique_ptr<MapMan> _mapMan;
		std::unique_ptr<MenuBar> _menuBar;
		std::unique_ptr<PlaceMode> _tilePlaceMode;
		std::unique_ptr<TexturePickMode> _texPickMode;
		std::unique_ptr<ShapePickMode> _shapePickMode;
		std::unique_ptr<EntMode> _entMode;
		ModeImpl* _editorMode = nullptr;
		Mode _currentMode = Mode::PLACE_TILE;

		std::filesystem::path _lastSavedPath;
		bool _previewDraw = false;
		bool _didSave = false;
		bool _quit = false;

		// Engine scene
		std::unique_ptr<scene::SceneManager> _sceneManager;
	};
} // namespace ed