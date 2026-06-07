#pragma once

#include <memory>
#include <deque>
#include <vector>
#include <string>
#include <filesystem>

#include "../tile.hpp"
#include "../ent.hpp"

namespace scene { class SceneNode; class ChunkNode; class SceneManager; }

namespace ed
{
	class MapMan final
	{
	public:
		// ---------- Action types for undo/redo ----------
		class Action
		{
		public:
			virtual ~Action() = default;
			virtual void Do(MapMan& map) const = 0;
			virtual void Undo(MapMan& map) const = 0;
		};

		class TileAction final : public Action
		{
		public:
			TileAction(size_t i, size_t j, size_t k, TileGrid prevState, TileGrid newState);
			void Do(MapMan& map) const override;
			void Undo(MapMan& map) const override;
		private:
			size_t _i, _j, _k;
			TileGrid _prevState, _newState;
		};

		class EntAction final : public Action
		{
		public:
			EntAction(size_t i, size_t j, size_t k, bool overwrite, bool removed,
				Ent oldEnt, Ent newEnt);
			void Do(MapMan& map) const override;
			void Undo(MapMan& map) const override;
		private:
			size_t _i, _j, _k;
			bool _overwrite, _removed;
			Ent _oldEnt, _newEnt;
		};

		// ---------- Construction ----------
		MapMan();
		~MapMan();

		void NewMap(int width, int height, int length);

		// ---------- Grid access ----------
		[[nodiscard]] const TileGrid& Tiles() const noexcept { return _tileGrid; }
		[[nodiscard]] const EntGrid& Ents() const noexcept { return _entGrid; }

		// ---------- Map operations ----------
		void ExpandMap(Direction axis, int amount);
		void ShrinkMap();

		// ---------- I/O ----------
		bool SaveTE3Map(const std::filesystem::path& filePath);
		bool LoadTE3Map(const std::filesystem::path& filePath);
		bool LoadTE2Map(const std::filesystem::path& filePath);
		bool ExportGLTFScene(const std::filesystem::path& filePath, bool separateGeometry);

		// ---------- Commands (with undo) ----------
		void ExecuteTileAction(size_t i, size_t j, size_t k, size_t w, size_t h, size_t l, Tile newTile);
		void ExecuteTileAction(size_t i, size_t j, size_t k, size_t w, size_t h, size_t l, TileGrid brush);
		void ExecuteEntPlacement(int i, int j, int k, Ent newEnt);
		void ExecuteEntRemoval(int i, int j, int k);

		// ---------- Asset ID management ----------
		TexID GetOrAddTexID(const std::filesystem::path& texturePath);
		ModelID GetOrAddModelID(const std::filesystem::path& modelPath);
		std::filesystem::path PathFromTexID(TexID id) const;
		std::filesystem::path PathFromModelID(ModelID id) const;

		[[nodiscard]] const std::vector<std::shared_ptr<class ModelHandle>>& GetModelList() const noexcept { return _modelList; }
		[[nodiscard]] const std::vector<std::filesystem::path>& GetModelPathList() const noexcept { return _modelPaths; }
		[[nodiscard]] int GetNumModels() const noexcept { return static_cast<int>(_modelList.size()); }

		[[nodiscard]] const std::vector<std::shared_ptr<class TexHandle>>& GetTextureList() const noexcept { return _textureList; }
		[[nodiscard]] const std::vector<std::filesystem::path>& GetTexturePathList() const noexcept { return _texturePaths; }
		[[nodiscard]] int GetNumTextures() const noexcept { return static_cast<int>(_textureList.size()); }

		// ---------- Default camera ----------
		[[nodiscard]] glm::vec3 GetDefaultCameraPosition() const noexcept { return _defaultCameraPosition; }
		void SetDefaultCameraPosition(glm::vec3 pos) noexcept { _defaultCameraPosition = pos; }
		[[nodiscard]] glm::vec3 GetDefaultCameraAngles() const noexcept { return _defaultCameraAngles; }
		void SetDefaultCameraAngles(glm::vec3 angles) noexcept { _defaultCameraAngles = angles; }

		// ---------- Undo / Redo ----------
		void Undo();
		void Redo();
		[[nodiscard]] bool HasUnsavedChanges() const noexcept { return _numberOfChanges != 0; }
		[[nodiscard]] bool WillConvert() const noexcept { return _willConvert; }

		// ---------- Scene sync ----------
		void SyncSceneGraph(scene::SceneNode& root, scene::SceneManager& sm);
		void ClearSceneGraph();
		[[nodiscard]] bool IsSceneGraphDirty() const noexcept { return _sceneGraphDirty; }

	private:
		void execute(std::shared_ptr<Action> action);

		TileGrid _tileGrid;
		EntGrid _entGrid;

		glm::vec3 _defaultCameraPosition = glm::vec3(0.0f);
		glm::vec3 _defaultCameraAngles = glm::vec3(0.0f);

		// Asset lists
		std::vector<std::shared_ptr<class TexHandle>> _textureList;
		std::vector<std::shared_ptr<class ModelHandle>> _modelList;
		std::vector<std::filesystem::path> _texturePaths;
		std::vector<std::filesystem::path> _modelPaths;

		// Undo / redo
		std::deque<std::shared_ptr<Action>> _undoHistory;
		std::deque<std::shared_ptr<Action>> _redoHistory;
		int32_t _numberOfChanges = 0;
		bool _willConvert = false;
		bool _sceneGraphDirty = false;
	};
} // namespace ed