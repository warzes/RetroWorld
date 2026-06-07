#pragma once

#include <memory>
#include <array>
#include <string>
#include <glm/glm.hpp>
#include "../editor_app.hpp"
#include "../tile.hpp"
#include "../ent.hpp"
#include <gr_camera.h>

namespace ed { class MapMan; class TexHandle; class ModelHandle; }

namespace ed
{
	struct Cursor
	{
		glm::vec3 position;
		glm::vec3 endPosition;
		virtual void Update(const glm::vec3& gridPos) = 0;
		virtual void Draw(const gr::Camera& camera) = 0;
		virtual ~Cursor() = default;
	};

	struct TileCursor final : Cursor
	{
		Tile tile;
		void Update(const glm::vec3& gridPos) override;
		void Draw(const gr::Camera& camera) override;
	};

	struct BrushCursor final : Cursor
	{
		explicit BrushCursor(class MapMan& mapMan);
		TileGrid brush;
		void Update(const glm::vec3& gridPos) override;
		void Draw(const gr::Camera& camera) override;
	};

	struct EntCursor final : Cursor
	{
		Ent ent;
		void Update(const glm::vec3& gridPos) override;
		void Draw(const gr::Camera& camera) override;
	};

	class PlaceMode final : public EditorApp::ModeImpl
	{
	public:
		explicit PlaceMode(MapMan& mapMan);
		~PlaceMode() override = default;

		void Update() override;
		void Draw() override;
		void OnEnter() override;
		void OnExit() override;

		void SetCursorShape(std::shared_ptr<ModelHandle> shape);
		void SetCursorTextures(std::array<std::shared_ptr<TexHandle>, TEXTURES_PER_TILE> tex);
		void SetCursorEnt(const Ent& ent);
		void SetShapeFromModel(const std::filesystem::path& modelPath);
		std::shared_ptr<ModelHandle> GetCursorShape() const;
		std::array<std::shared_ptr<TexHandle>, TEXTURES_PER_TILE> GetCursorTextures() const;
		const Ent& GetCursorEnt() const;
		[[nodiscard]] glm::vec3 GetCursorWorldPos() const noexcept { return _cursorWorldPos; }
		[[nodiscard]] glm::vec3 GetCursorEndWorldPos() const noexcept { return _cursorWorldEndPos; }
		[[nodiscard]] bool HasActiveCursor() const noexcept { return _cursor != nullptr; }

		void ResetCamera();
		glm::vec3 GetCameraPosition() const;
		glm::vec3 GetCameraAngles() const;
		void SetCameraOrientation(glm::vec3 position, glm::vec3 angles);
		void ResetGrid();

	private:
		MapMan& _mapMan;
		gr::Camera _camera;
		float _cameraYaw = 0.0f, _cameraPitch = 0.0f, _cameraMoveSpeed = 10.0f;
		TileCursor _tileCursor;
		BrushCursor _brushCursor;
		EntCursor _entCursor;
		Cursor* _cursor = nullptr;
		glm::vec3 _cursorPreviousGridPos = glm::vec3(0.0f);
		glm::vec2 _previousMousePosition = glm::vec2(0.0f);
		float _outlineScale = 1.0f;
		int _layerViewMin = -100, _layerViewMax = 100;
		glm::vec3 _planeGridPos = glm::vec3(0.0f), _planeWorldPos = glm::vec3(0.0f);
		glm::vec3 _cursorWorldPos = glm::vec3(0.0f);
		glm::vec3 _cursorWorldEndPos = glm::vec3(0.0f);
		bool _multiSelectActive = false;
		glm::vec3 _multiSelectStartGrid = glm::vec3(0.0f);
		std::shared_ptr<ModelHandle> _cursorShape;
		std::array<std::shared_ptr<TexHandle>, TEXTURES_PER_TILE> _cursorTextures;
		Ent _cursorEnt;

		void handleInput();
		void updateCamera();
		void drawUI();
		void placeTile();
	};
} // namespace ed
