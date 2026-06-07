#include "stdafx.h"
#include "place_mode.hpp"
#include "../map_man/map_man.hpp"
#include "../editor_app.hpp"
#include "../assets.hpp"
#include <imgui/imgui.h>
#include <app_input.h>
#include <app_keys.h>
//=============================================================================
namespace
{
	// Shared cursor assets (created once, reused)
	std::shared_ptr<gr::Mesh> g_cursorBox;
	std::shared_ptr<gr::Material> g_cursorMat;
	std::shared_ptr<gr::Material> g_brushCursorMat;
	std::shared_ptr<gr::Material> g_entCursorMat;
	std::shared_ptr<gr::Mesh> g_cursorSphere;

	void EnsureCursorAssets()
	{
		if (g_cursorBox) return;

		g_cursorBox = std::make_shared<gr::Mesh>(gr::Mesh::CreateCube());

		g_cursorMat = std::make_shared<gr::Material>();
		g_cursorMat->albedoColor = glm::vec3(1.0f, 1.0f, 1.0f);
		g_cursorMat->ambientColor = glm::vec3(1.0f);
		g_cursorMat->specularColor = glm::vec3(0.0f);
		g_cursorMat->shininess = 1.0f;
		g_cursorMat->opacity = 0.4f;
		g_cursorMat->cullMode = gpu::CullMode::None;
		g_cursorMat->castShadow = false;
		g_cursorMat->receiveShadow = false;

		g_brushCursorMat = std::make_shared<gr::Material>();
		g_brushCursorMat->albedoColor = glm::vec3(0.2f, 0.8f, 0.2f);
		g_brushCursorMat->ambientColor = glm::vec3(1.0f);
		g_brushCursorMat->specularColor = glm::vec3(0.0f);
		g_brushCursorMat->shininess = 1.0f;
		g_brushCursorMat->opacity = 0.3f;
		g_brushCursorMat->cullMode = gpu::CullMode::None;
		g_brushCursorMat->castShadow = false;
		g_brushCursorMat->receiveShadow = false;

		g_entCursorMat = std::make_shared<gr::Material>();
		g_entCursorMat->albedoColor = glm::vec3(1.0f, 0.9f, 0.3f);
		g_entCursorMat->ambientColor = glm::vec3(1.0f);
		g_entCursorMat->specularColor = glm::vec3(0.0f);
		g_entCursorMat->shininess = 1.0f;
		g_entCursorMat->opacity = 0.35f;
		g_entCursorMat->cullMode = gpu::CullMode::None;
		g_entCursorMat->castShadow = false;
		g_entCursorMat->receiveShadow = false;

		g_cursorSphere = std::make_shared<gr::Mesh>(gr::Mesh::CreateSphere(8, 12));
	}
}
//=============================================================================
ed::PlaceMode::PlaceMode(MapMan& mapMan)
	: _mapMan(mapMan)
	, _camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f))
	, _brushCursor(mapMan)
{}
//=============================================================================
void ed::PlaceMode::OnEnter()
{
	_cursor = &_tileCursor;

	if (!_cursorShape)
	{
		// Load defaults from settings
		auto& app = EditorApp::Get();
		std::string defaultTexPath = app.GetDefaultTexturePath();
		if (!defaultTexPath.empty())
		{
			for (auto& tex : _cursorTextures)
				tex = Assets::GetTexture(defaultTexPath);
		}
		std::string defaultShapePath = app.GetDefaultShapePath();
		if (!defaultShapePath.empty())
			_cursorShape = Assets::GetModel(defaultShapePath);
	}

	// Sync cursor tile state
	if (_cursorShape)
	{
		_tileCursor.tile.shape = EditorApp::Get().GetMapMan().GetOrAddModelID(
			_cursorShape->GetPath());
	}
	for (size_t i = 0; i < TEXTURES_PER_TILE; ++i)
	{
		if (_cursorTextures[i])
			_tileCursor.tile.textures[i] = EditorApp::Get().GetMapMan().GetOrAddTexID(
				_cursorTextures[i]->GetPath());
	}

	_cameraYaw = -135.0f;
	_cameraPitch = -35.0f;

	auto& tiles = _mapMan.Tiles();
	_planeGridPos = glm::vec3(
		static_cast<float>(tiles.GetWidth()) * 0.5f,
		0.0f,
		static_cast<float>(tiles.GetLength()) * 0.5f);
	_planeWorldPos = tiles.GridToWorldPos(_planeGridPos, false);
}
//=============================================================================
void ed::PlaceMode::OnExit()
{
	// Remove cursor node from scene graph
	auto& sm = EditorApp::Get().GetSceneManager();
	if (sm.root)
		sm.root->RemoveChild("cursor");
}
//=============================================================================
void ed::PlaceMode::Update()
{
	auto& app = EditorApp::Get();
	auto& sceneMan = app.GetSceneManager();
	auto& tiles = _mapMan.Tiles();
	float spacing = tiles.GetSpacing();

	glm::vec3 cursorWorldPos(0.0f);

	if (_cursor)
	{
		auto& io = ImGui::GetIO();
		if (!io.WantCaptureMouse)
		{
			// Mouse raycast onto editing plane
			auto mousePos = input::GetMousePosition();
			float winW = static_cast<float>(window::GetWidth());
			float winH = static_cast<float>(window::GetHeight());

			float ndcX = (2.0f * static_cast<float>(mousePos.x)) / winW - 1.0f;
			float ndcY = 1.0f - (2.0f * static_cast<float>(mousePos.y)) / winH;

			glm::vec3 rayOrigin(0.0f);
			glm::vec3 rayDir(0.0f, -1.0f, 0.0f);

			if (sceneMan.activeCamera)
			{
				glm::mat4 invVP = glm::inverse(sceneMan.activeCamera->GetViewProjectionMatrix());
				glm::vec4 nearP = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
				glm::vec4 farP = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
				nearP /= nearP.w;
				farP /= farP.w;
				rayOrigin = glm::vec3(nearP);
				rayDir = glm::normalize(glm::vec3(farP) - rayOrigin);
			}

			if (glm::abs(rayDir.y) > 1e-6f)
			{
				float planeY = _planeWorldPos.y;
				float t = (planeY - rayOrigin.y) / rayDir.y;
				if (t >= 0.0f)
				{
					glm::vec3 worldPos = rayOrigin + t * rayDir;
					glm::vec3 gridPos = tiles.WorldToGridPos(worldPos);
					glm::vec3 snapped = tiles.GridToWorldPos(gridPos, true);
					snapped.y = _planeWorldPos.y + spacing * 0.5f;

					// Shift+drag multi-select
					bool shift = input::IsKeyDown(KeyboardType::KEY_LEFT_SHIFT);
					if (!shift)
					{
						_multiSelectActive = false;
						_cursor->position = snapped;
						_cursor->endPosition = snapped;
						_cursorPreviousGridPos = gridPos;
					}
					else if (_cursor == &_tileCursor)
					{
						if (!_multiSelectActive)
						{
							_multiSelectActive = true;
							_multiSelectStartGrid = gridPos;
							_cursor->endPosition = snapped;
						}
						_cursor->position = snapped;
						_cursorPreviousGridPos = gridPos;
					}

					cursorWorldPos = snapped;
					_cursorWorldPos = _cursor->position;
					_cursorWorldEndPos = _cursor->endPosition;
				}
			}

			// Scroll wheel changes the editing plane height
			float wheel = io.MouseWheel;
			if (glm::abs(wheel) > 0.0f)
			{
				_planeGridPos.y = glm::clamp(
					_planeGridPos.y + (wheel > 0.0f ? 1.0f : -1.0f),
					0.0f, static_cast<float>(tiles.GetHeight() - 1));
				_planeWorldPos = tiles.GridToWorldPos(_planeGridPos, false);
			}
		}
	}

	// Handle input (placement uses _cursorPreviousGridPos set above)
	handleInput();
	updateCamera();

	// Update grid Y position to match editing plane
	if (sceneMan.root)
	{
		auto* gridNode = sceneMan.root->FindChild("grid");
		if (gridNode)
		{
			gridNode->transform.position.y = _planeWorldPos.y;
		}
	}

	// Update cursor scene node
	EnsureCursorAssets();
	if (sceneMan.root)
	{
		sceneMan.root->RemoveChild("cursor");

		if (_cursor)
		{
			auto& cursorNode = sceneMan.root->AddChild<scene::ModelNode>("cursor");
			cursorNode.castShadow = false;
			cursorNode.receiveShadow = false;

			if (_cursor == &_tileCursor)
			{
				if (_cursorShape)
					cursorNode.mesh = _cursorShape->GetMesh();
				else
					cursorNode.mesh = g_cursorBox;

				cursorNode.material = g_cursorMat;

				std::shared_ptr<gr::Mesh> tileMesh;
				if (_cursorShape)
					tileMesh = _cursorShape->GetMesh();
				if (tileMesh)
					cursorNode.mesh = tileMesh;
				else
					cursorNode.mesh = g_cursorBox;

				// Use tile texture if available
				if (_cursorTextures[0])
					cursorNode.material->albedoMap = _cursorTextures[0]->GetTexture();

				if (_multiSelectActive)
				{
					glm::vec3 minP = glm::min(_cursor->position, _cursor->endPosition);
					glm::vec3 maxP = glm::max(_cursor->position, _cursor->endPosition);
					glm::vec3 center = (minP + maxP) * 0.5f;
					center.y += 0.05f;
					glm::vec3 size = (maxP - minP) + glm::vec3(spacing);
					cursorNode.transform.position = center;
					cursorNode.transform.scale = size;
				}
				else
				{
					cursorWorldPos.y += 0.05f;
					cursorNode.transform.position = cursorWorldPos;
				}
			}
			else if (_cursor == &_brushCursor)
			{
				cursorNode.mesh = g_cursorBox;
				cursorNode.material = g_brushCursorMat;

				float sx = static_cast<float>(_brushCursor.brush.GetWidth()) * spacing;
				float sy = static_cast<float>(_brushCursor.brush.GetHeight()) * spacing;
				float sz = static_cast<float>(_brushCursor.brush.GetLength()) * spacing;
				cursorNode.transform.scale = glm::vec3(sx, sy, sz);

				// Center the box on the brush area
				glm::vec3 brushCenter = cursorWorldPos + glm::vec3(sx, sy, sz) * 0.5f;
				cursorNode.transform.position = brushCenter;
			}
			else if (_cursor == &_entCursor)
			{
				cursorNode.mesh = g_cursorSphere;
				cursorNode.material = g_entCursorMat;

				float r = _cursorEnt.radius > 0.0f ? _cursorEnt.radius : 1.0f;
				cursorNode.transform.scale = glm::vec3(r);
				cursorNode.transform.position = cursorWorldPos;
			}
		}
	}
}
//=============================================================================
void ed::PlaceMode::Draw()
{
	drawUI();
	if (_cursor)
		_cursor->Draw(_camera);
}
//=============================================================================
void ed::PlaceMode::handleInput()
{
	auto& io = ImGui::GetIO();
	auto& app = EditorApp::Get();
	float dt = app::GetDeltaTime();

	if (!io.WantCaptureKeyboard)
	{
		float speed = _cameraMoveSpeed * dt;
		if (input::IsKeyDown(KeyboardType::KEY_W)) _camera.Move(gr::Movement::Forward, speed);
		if (input::IsKeyDown(KeyboardType::KEY_S)) _camera.Move(gr::Movement::Backward, speed);
		if (input::IsKeyDown(KeyboardType::KEY_A)) _camera.Move(gr::Movement::Left, speed);
		if (input::IsKeyDown(KeyboardType::KEY_D)) _camera.Move(gr::Movement::Right, speed);
		if (input::IsKeyDown(KeyboardType::KEY_Q)) _camera.Move(gr::Movement::Down, speed);
		if (input::IsKeyDown(KeyboardType::KEY_E)) _camera.Move(gr::Movement::Up, speed);
	}

	if (!io.WantCaptureMouse)
	{
		auto mousePos = input::GetMousePosition();
		glm::vec2 currentMouse(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y));

		if (input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
		{
			glm::vec2 delta = currentMouse - _previousMousePosition;
			float sens = app.GetMouseSensitivity();
			_cameraYaw += delta.x * sens;
			_cameraPitch += delta.y * sens;
			_cameraPitch = glm::clamp(_cameraPitch, -89.0f, 89.0f);
			_camera.Rotate(_cameraPitch, _cameraYaw, 0.0f);
		}

		if (input::IsMouseDown(MouseType::MOUSE_BUTTON_LEFT))
		{
			placeTile();
		}

		_previousMousePosition = currentMouse;
	}
}
//=============================================================================
void ed::PlaceMode::updateCamera()
{}
//=============================================================================
void ed::PlaceMode::drawUI()
{
	ImGui::Begin("Place Mode", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SeparatorText("Layer View");
	ImGui::SliderInt("Min Layer", &_layerViewMin, -100, 100);
	ImGui::SliderInt("Max Layer", &_layerViewMax, -100, 100);

	ImGui::SeparatorText("Cursor");
	if (_cursor)
	{
		ImGui::Text("Grid Pos: (%.0f, %.0f, %.0f)",
			_cursorPreviousGridPos.x, _cursorPreviousGridPos.y, _cursorPreviousGridPos.z);
	}
	ImGui::DragFloat("Outline Scale", &_outlineScale, 0.1f, 0.1f, 10.0f);
	ImGui::DragFloat("Camera Speed", &_cameraMoveSpeed, 0.5f, 1.0f, 100.0f);

	ImGui::SeparatorText("Tile Cursor");
	ImGui::Text("Shape ID: %d", _tileCursor.tile.shape);
	ImGui::Text("Tex IDs: %d, %d", _tileCursor.tile.textures[0], _tileCursor.tile.textures[1]);

	ImGui::End();
}
//=============================================================================
void ed::PlaceMode::placeTile()
{
	auto& tiles = _mapMan.Tiles();

	glm::vec3 startGrid, endGrid;
	if (_multiSelectActive && _cursor == &_tileCursor)
	{
		glm::vec3 curGrid = tiles.WorldToGridPos(_cursor->position);
		startGrid = glm::min(_multiSelectStartGrid, curGrid);
		endGrid = glm::max(_multiSelectStartGrid, curGrid);
	}
	else
	{
		startGrid = _cursorPreviousGridPos;
		endGrid = _cursorPreviousGridPos;
	}

	int i = static_cast<int>(startGrid.x);
	int j = static_cast<int>(startGrid.y);
	int k = static_cast<int>(startGrid.z);
	int w = static_cast<int>(endGrid.x - startGrid.x) + 1;
	int h = static_cast<int>(endGrid.y - startGrid.y) + 1;
	int l = static_cast<int>(endGrid.z - startGrid.z) + 1;

	if (i < 0 || j < 0 || k < 0 ||
		static_cast<size_t>(i) >= tiles.GetWidth() ||
		static_cast<size_t>(j) >= tiles.GetHeight() ||
		static_cast<size_t>(k) >= tiles.GetLength())
		return;
	if (i + w > static_cast<int>(tiles.GetWidth()) ||
		j + h > static_cast<int>(tiles.GetHeight()) ||
		k + l > static_cast<int>(tiles.GetLength()))
		return;

	// Early exit for single cell if tile is same (after bounds check)
	if (!_multiSelectActive || _cursor != &_tileCursor)
	{
		Tile under = tiles.GetTile(
			static_cast<size_t>(i),
			static_cast<size_t>(j),
			static_cast<size_t>(k));
		if (under == _tileCursor.tile)
			return;
	}

	_mapMan.ExecuteTileAction(
		static_cast<size_t>(i),
		static_cast<size_t>(j),
		static_cast<size_t>(k),
		static_cast<size_t>(w),
		static_cast<size_t>(h),
		static_cast<size_t>(l),
		_tileCursor.tile);
}
//=============================================================================
void ed::PlaceMode::ResetCamera()
{
	_camera = gr::Camera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_cameraYaw = -135.0f;
	_cameraPitch = -35.0f;
}
//=============================================================================
glm::vec3 ed::PlaceMode::GetCameraPosition() const
{
	return _camera.GetPosition();
}
//=============================================================================
glm::vec3 ed::PlaceMode::GetCameraAngles() const
{
	return glm::vec3(_cameraPitch, _cameraYaw, 0.0f);
}
//=============================================================================
void ed::PlaceMode::SetCameraOrientation(glm::vec3 position, glm::vec3 angles)
{
	_camera = gr::Camera(position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	_cameraYaw = angles.y;
	_cameraPitch = angles.x;
	_camera.Rotate(_cameraPitch, _cameraYaw, 0.0f);
}
//=============================================================================
void ed::PlaceMode::ResetGrid()
{
	_outlineScale = 1.125f;
	_layerViewMin = 0;
	_layerViewMax = static_cast<int>(_mapMan.Tiles().GetHeight() - 1);

	auto& tiles = _mapMan.Tiles();
	_planeGridPos = glm::vec3(
		static_cast<float>(tiles.GetWidth()) * 0.5f,
		0.0f,
		static_cast<float>(tiles.GetLength()) * 0.5f);
	_planeWorldPos = tiles.GridToWorldPos(_planeGridPos, false);
}
//=============================================================================
void ed::PlaceMode::SetCursorShape(std::shared_ptr<ModelHandle> shape)
{
	_cursorShape = shape;
	_cursor = &_tileCursor;
}
//=============================================================================
void ed::PlaceMode::SetShapeFromModel(const std::filesystem::path& modelPath)
{
	auto shape = Assets::GetModel(modelPath);
	_cursorShape = shape;
	_cursor = &_tileCursor;
	if (shape)
	{
		ModelID id = EditorApp::Get().GetMapMan().GetOrAddModelID(modelPath);
		_tileCursor.tile.shape = id;
	}
}
//=============================================================================
void ed::PlaceMode::SetCursorTextures(std::array<std::shared_ptr<TexHandle>, TEXTURES_PER_TILE> tex)
{
	_cursorTextures = tex;
	_cursor = &_tileCursor;
	for (size_t i = 0; i < TEXTURES_PER_TILE; ++i)
	{
		if (_cursorTextures[i])
			_tileCursor.tile.textures[i] = EditorApp::Get().GetMapMan().GetOrAddTexID(
				_cursorTextures[i]->GetPath());
	}
}
//=============================================================================
void ed::PlaceMode::SetCursorEnt(const Ent& ent)
{
	_cursorEnt = ent;
	_cursor = &_entCursor;
}
//=============================================================================
std::shared_ptr<ed::ModelHandle> ed::PlaceMode::GetCursorShape() const
{
	return _cursorShape;
}
//=============================================================================
std::array<std::shared_ptr<ed::TexHandle>, ed::TEXTURES_PER_TILE> ed::PlaceMode::GetCursorTextures() const
{
	return _cursorTextures;
}
//=============================================================================
const ed::Ent& ed::PlaceMode::GetCursorEnt() const
{
	return _cursorEnt;
}
//=============================================================================
