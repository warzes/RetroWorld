#include "stdafx.h"
#include "editor_app.hpp"
#include "map_man/map_man.hpp"
#include "menu_bar.hpp"
#include "assets.hpp"
#include "place_mode/place_mode.hpp"
#include "pick_mode/pick_mode.hpp"
#include "pick_mode/pick_mode.hpp"
#include "ent_mode/ent_mode.hpp"
#include <imgui/imgui.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <app_mouseLook.h>
//=============================================================================
namespace fs = std::filesystem;
//=============================================================================
// Embedded shader sources (from original EditorApp.cpp)
namespace
{
#include "assets/shaders/map_shader.hpp"
#include "assets/shaders/sprite_shader.hpp"
}
//=============================================================================
// Global engine resources
// LightBlockUBO compatible with blinnPhongFrag std140 layout
struct alignas(16) EditorLightDataGPU
{
	glm::vec4 positionOrDirection;
	glm::vec3 color;
	float     intensity;
	glm::vec3 attenuation;
	float     radius;
	glm::vec3 spotDirection;
	float     innerCutoff;
	float     outerCutoff;
	int32_t   type;
	int32_t   castShadow;
	float     shadowBias;
	glm::mat4 lightSpaceMatrix;
};
static_assert(sizeof(EditorLightDataGPU) == 144);

struct alignas(16) LightBlockUBO
{
	int32_t      lightCount;
	uint8_t      _pad[12];
	EditorLightDataGPU lights[16];
};
static_assert(sizeof(LightBlockUBO) == 4 + 12 + 144 * 16);

namespace
{
	gpu::program::ShaderProgramPtr g_blinnPhongProgram;
	gpu::program::ShaderProgramPtr g_depthShader;
	gpu::program::ShaderProgramPtr g_pointDepthShader;
	gpu::program::ShaderProgramPtr g_mapShader;
	gpu::program::ShaderProgramPtr g_spriteShader;
	input::MouseLook g_mouseLook;
	gr::Camera g_editorCamera(glm::vec3(10.0f, 10.0f, 10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

//=============================================================================
gpu::program::ShaderProgramPtr ed::EditorApp::GetBlinnPhongProgram()
{
	return g_blinnPhongProgram;
}
//=============================================================================
// App lifecycle callbacks
bool EditorGameInit();
void EditorGameClose();
void EditorGameUpdate();
void EditorGameRender();
void EditorGameRenderUI();
//=============================================================================
void ed::EditorApp::Settings::to_json(nlohmann::json& j, const Settings& s)
{
	j = nlohmann::json::object({
		{"texturesDir", s.texturesDir},
		{"shapesDir", s.shapesDir},
		{"undoMax", s.undoMax},
		{"mouseSensitivity", s.mouseSensitivity},
		{"exportSeparateGeometry", s.exportSeparateGeometry},
		{"cullFaces", s.cullFaces},
		{"exportFilePath", s.exportFilePath},
		{"defaultTexturePath", s.defaultTexturePath},
		{"defaultShapePath", s.defaultShapePath},
		{"backgroundColor", nlohmann::json::array({
			s.backgroundColor.x, s.backgroundColor.y, s.backgroundColor.z })},
		{"assetHideRegex", s.assetHideRegex}
		});
}
//=============================================================================
void ed::EditorApp::Settings::from_json(const nlohmann::json& j, Settings& s)
{
	s.texturesDir = j.value("texturesDir", "data/assets/textures/tiles");
	s.shapesDir = j.value("shapesDir", "data/assets/models/shapes");
	s.undoMax = j.value("undoMax", 64u);
	s.mouseSensitivity = j.value("mouseSensitivity", 0.002f);
	s.exportSeparateGeometry = j.value("exportSeparateGeometry", false);
	s.cullFaces = j.value("cullFaces", true);
	s.exportFilePath = j.value("exportFilePath", "");
	s.defaultTexturePath = j.value("defaultTexturePath", "");
	s.defaultShapePath = j.value("defaultShapePath", "");
	s.assetHideRegex = j.value("assetHideRegex", "");
	if (j.contains("backgroundColor"))
	{
		auto& arr = j["backgroundColor"];
		s.backgroundColor = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
	}
}
//=============================================================================
ed::EditorApp::EditorApp()
	: _mapMan(std::make_unique<MapMan>())
{}
//=============================================================================
ed::EditorApp::~EditorApp() = default;
//=============================================================================
void ed::EditorApp::InitGameSystems()
{
	// Scene manager
	_sceneManager = std::make_unique<scene::SceneManager>();
	auto& root = *_sceneManager->root;

	// Camera
	auto& cam = root.AddChild<scene::CameraNode>("editor_camera");
	cam.aspectRatio = window::GetAspectRatio();
	cam.externalCamera = &g_editorCamera;
	_sceneManager->SetActiveCamera(cam);

	// Default directional light
	auto& sun = root.AddChild<scene::LightNode>("sun");
	sun.lightType = scene::LightNode::LightType::Directional;
	sun.color = glm::vec3(1.0f, 0.95f, 0.85f);
	sun.intensity = 1.2f;
	sun.castShadow = true;
	sun.shadowSettings.resolution = 2048;
	sun.shadowSettings.orthoSize = 50.0f;
	sun.shadowSettings.cascadeDistance[0] = -1.0f;

	_sceneManager->enableShadows = true;
	_sceneManager->enableInstancing = true;

	// Create mode instances
	_tilePlaceMode = std::make_unique<ed::PlaceMode>(*_mapMan);
	_texPickMode = std::make_unique<ed::TexturePickMode>();
	_shapePickMode = std::make_unique<ed::ShapePickMode>();
	_entMode = std::make_unique<ed::EntMode>();

	// Menu bar
	_menuBar = std::make_unique<ed::MenuBar>(_settings, *_mapMan);
}
//=============================================================================
void ed::EditorApp::ClearGameSystems()
{
	_entMode.reset();
	_shapePickMode.reset();
	_texPickMode.reset();
	_tilePlaceMode.reset();
	_menuBar.reset();
	_sceneManager.reset();
}
//=============================================================================
void ed::EditorApp::Init(int argc, char* argv[])
{
	// Run the engine with our callbacks
	app::AppCreateInfo createInfo{};
	createInfo.init_cb = EditorGameInit;
	createInfo.close_cb = EditorGameClose;
	createInfo.update_cb = EditorGameUpdate;
	createInfo.fixedUpdate_cb = nullptr;
	createInfo.render_cb = EditorGameRender;
	createInfo.renderUi_cb = EditorGameRenderUI;

	// Pass command line map file if provided
	if (argc > 1)
	{
		_lastSavedPath = argv[1];
	}

	app::Run(createInfo);
}
//=============================================================================
void ed::EditorApp::ChangeEditorMode(Mode newMode)
{
	if (_editorMode)
		_editorMode->OnExit();

	switch (newMode)
	{
	case Mode::PLACE_TILE:
		_editorMode = _tilePlaceMode.get();
		break;
	case Mode::PICK_TEXTURE:
		_editorMode = _texPickMode.get();
		break;
	case Mode::PICK_SHAPE:
		_editorMode = _shapePickMode.get();
		break;
	case Mode::EDIT_ENT:
		_editorMode = _entMode.get();
		break;
	}
	_currentMode = newMode;

	if (_editorMode)
		_editorMode->OnEnter();
}
//=============================================================================
void ed::EditorApp::ResetEditorCamera()
{
	if (_tilePlaceMode)
		_tilePlaceMode->ResetCamera();
}
//=============================================================================
void ed::EditorApp::NewMap(int width, int height, int length)
{
	_mapMan->NewMap(width, height, length);
	_lastSavedPath.clear();
	_didSave = false;
	SyncMapToSceneGraph();
}
//=============================================================================
void ed::EditorApp::ExpandMap(Direction axis, int amount)
{
	_mapMan->ExpandMap(axis, amount);
	SyncMapToSceneGraph();
}
//=============================================================================
void ed::EditorApp::ShrinkMap()
{
	_mapMan->ShrinkMap();
	SyncMapToSceneGraph();
}
//=============================================================================
void ed::EditorApp::TryOpenMap(const fs::path& path)
{
	if (!fs::exists(path))
	{
		DisplayStatusMessage("File not found: " + path.string(), 5.0f, 1);
		return;
	}

	auto ext = path.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(),
		[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

	bool loaded = false;
	if (ext == ".te3")
		loaded = _mapMan->LoadTE3Map(path);
	else if (ext == ".te2" || ext == ".json")
		loaded = _mapMan->LoadTE2Map(path);

	if (loaded)
	{
		_lastSavedPath = path;
		if (_mapMan->WillConvert())
			DisplayStatusMessage("Converted from old format", 3.0f, 0);
		else
			DisplayStatusMessage("Opened: " + path.filename().string(), 3.0f, 0);

		SyncMapToSceneGraph();
	}
	else
	{
		DisplayStatusMessage("Failed to open map", 5.0f, 2);
	}
}
//=============================================================================
void ed::EditorApp::TrySaveMap(const fs::path& path)
{
	if (_mapMan->SaveTE3Map(path))
	{
		_lastSavedPath = path;
		_didSave = true;
		DisplayStatusMessage("Saved: " + path.filename().string(), 3.0f, 0);
	}
	else
	{
		DisplayStatusMessage("Failed to save map", 5.0f, 2);
	}
}
//=============================================================================
void ed::EditorApp::TryExportMap(const fs::path& path, bool separateGeometry)
{
	if (_mapMan->ExportGLTFScene(path, separateGeometry))
	{
		DisplayStatusMessage("Exported: " + path.filename().string(), 3.0f, 0);
	}
	else
	{
		DisplayStatusMessage("Failed to export map", 5.0f, 2);
	}
}
//=============================================================================
void ed::EditorApp::SaveSettings()
{
	nlohmann::json j;
	Settings::to_json(j, _settings);

	fs::path configPath = fs::current_path() / "editor_settings.json";
	std::ofstream fout(configPath);
	if (fout.is_open())
		fout << j.dump(1, '\t') << std::endl;
}
//=============================================================================
void ed::EditorApp::LoadSettings()
{
	fs::path configPath = fs::current_path() / "editor_settings.json";
	if (!fs::exists(configPath)) return;

	std::ifstream fin(configPath);
	if (!fin.is_open()) return;

	nlohmann::json j;
	try { fin >> j; Settings::from_json(j, _settings); }
	catch (...) {}

	// Validate saved paths – fall back to defaults if they don't exist
	if (!fs::exists(_settings.texturesDir))
		_settings.texturesDir = "data/assets/textures/tiles";
	if (!fs::exists(_settings.shapesDir))
		_settings.shapesDir = "data/assets/models/shapes";
}
//=============================================================================
void ed::EditorApp::DisplayStatusMessage(std::string message, float durationSeconds, int priority)
{
	if (_menuBar)
		_menuBar->DisplayStatusMessage(std::move(message), durationSeconds, priority);
}
//=============================================================================
void ed::EditorApp::Update()
{
	// Update current editor mode
	if (_editorMode)
		_editorMode->Update();

	// Sync scene graph if map was modified
	if (_mapMan->IsSceneGraphDirty())
		SyncMapToSceneGraph();

	// Restore grid Y after sync (SyncSceneGraph recreates grid at y=0)
	if (_sceneManager && _sceneManager->root)
	{
		auto* gridNode = _sceneManager->root->FindChild("grid");
		if (gridNode && _editorMode)
		{
			auto* placeMode = dynamic_cast<ed::PlaceMode*>(_editorMode);
			if (placeMode)
				gridNode->transform.position.y = placeMode->GetPlaneWorldPos().y;
		}
	}

	// Update scene manager
	if (_sceneManager)
	{
		// Sync camera aspect ratio
		if (_sceneManager->activeCamera)
			_sceneManager->activeCamera->aspectRatio = window::GetAspectRatio();

		_sceneManager->Update();
	}

	// Update menu bar timer
	if (_menuBar)
		_menuBar->Update();
}
//=============================================================================
void ed::EditorApp::SyncMapToSceneGraph()
{
	if (!_sceneManager) return;
	_mapMan->SyncSceneGraph(*_sceneManager->root, *_sceneManager);
}
//=============================================================================
// Engine lifecycle
bool EditorGameInit()
{
	// Load settings
	ed::EditorApp::Get().LoadSettings();

	// Create shader programs
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "BlinnPhong",
			.vertexShaderCode = g_blinnPhongVert,
			.fragmentShaderCode = g_blinnPhongFrag
		};
		g_blinnPhongProgram = gpu::program::CreateShaderProgram(info);
		if (!gpu::program::IsValid(g_blinnPhongProgram))
		{
			core::Error("Editor: failed to compile BlinnPhong shader");
			return false;
		}
	}
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "ShadowDepth",
			.vertexShaderCode = g_shadowDepthVert,
			.fragmentShaderCode = g_shadowDepthFrag
		};
		g_depthShader = gpu::program::CreateShaderProgram(info);
	}
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "PointShadowDepth",
			.vertexShaderCode = g_pointShadowDepthVert,
			.fragmentShaderCode = g_pointShadowDepthFrag,
			.geometryShaderCode = g_pointShadowDepthGeom
		};
		g_pointDepthShader = gpu::program::CreateShaderProgram(info);
	}
	{
		// Map shader for rendering tiles
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "MapShader",
			.vertexShaderCode = g_mapShaderVert,
			.fragmentShaderCode = g_mapShaderFrag
		};
		g_mapShader = gpu::program::CreateShaderProgram(info);
	}
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "SpriteShader",
			.vertexShaderCode = g_spriteShaderVert,
			.fragmentShaderCode = g_spriteShaderFrag
		};
		g_spriteShader = gpu::program::CreateShaderProgram(info);
	}

	auto& app = ed::EditorApp::Get();

	// Init assets
	ed::Assets::Init(app.GetSettings().texturesDir, app.GetSettings().shapesDir);

	// Scene manager
	app.InitGameSystems();

	// Start in place tile mode
	app.ChangeEditorMode(ed::EditorApp::Mode::PLACE_TILE);

	// Create new map or open from command line
	if (app.GetLastSavedPath().empty())
		app.NewMap(100, 5, 100);
	else
		app.TryOpenMap(app.GetLastSavedPath());

	return true;
}
//=============================================================================
void EditorGameClose()
{
	auto& app = ed::EditorApp::Get();
	app.SaveSettings();
	app.ClearGameSystems();

	g_blinnPhongProgram.reset();
	g_depthShader.reset();
	g_pointDepthShader.reset();
	g_mapShader.reset();
	g_spriteShader.reset();
	g_mouseLook.Reset();
}
//=============================================================================
void EditorGameUpdate()
{
	auto& app = ed::EditorApp::Get();
	float dt = app::GetDeltaTime();

	// Camera movement (WASD)
	float speed = 15.0f * dt;
	if (input::IsKeyDown(KeyboardType::KEY_W)) g_editorCamera.Move(gr::Movement::Forward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_S)) g_editorCamera.Move(gr::Movement::Backward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_A)) g_editorCamera.Move(gr::Movement::Left, speed);
	if (input::IsKeyDown(KeyboardType::KEY_D)) g_editorCamera.Move(gr::Movement::Right, speed);
	if (input::IsKeyDown(KeyboardType::KEY_Q)) g_editorCamera.Move(gr::Movement::Down, speed);
	if (input::IsKeyDown(KeyboardType::KEY_E)) g_editorCamera.Move(gr::Movement::Up, speed);

	// Update editor
	app.Update();

	// Mouse look
	if (!ImGui::GetIO().WantCaptureMouse && input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
		g_mouseLook.OnRightDown();
	else
		g_mouseLook.OnRightUp();
	g_mouseLook.Update(g_editorCamera);
}
//=============================================================================
void EditorGameRender()
{
	auto& sceneMan = ed::EditorApp::Get().GetSceneManager();

	math::Frustum frustum;
	if (sceneMan.activeCamera)
		frustum = sceneMan.activeCamera->ExtractFrustum();

	// Shadow passes
	if (sceneMan.enableShadows)
	{
		for (auto* light : sceneMan.lights)
		{
			if (!light->castShadow) continue;
			auto shadowQueue = sceneMan.BuildRenderQueue(frustum, scene::RenderPassType::Shadow);
			if (light->lightType == scene::LightNode::LightType::Point)
				sceneMan.RenderShadowPass(shadowQueue, *light, g_depthShader, g_pointDepthShader);
			else
				sceneMan.RenderShadowPass(shadowQueue, *light, g_depthShader);
		}
	}

	// Main render pass
	glm::vec3 bg = ed::EditorApp::Get().GetBackgroundColor();
	gpu::fbo::SwapchainRenderInfo swapchainRI = {};
	swapchainRI.colorLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.clearColorValue[0] = bg.x;
	swapchainRI.clearColorValue[1] = bg.y;
	swapchainRI.clearColorValue[2] = bg.z;
	swapchainRI.clearColorValue[3] = 1.0f;
	swapchainRI.depthLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.viewport.drawRect.offset = { 0, 0 };
	swapchainRI.viewport.drawRect.extent = { window::GetWidth(), window::GetHeight() };

	gpu::cmd::BeginDraw(swapchainRI, "EditorFrame");

	sceneMan.enableFrustumCulling = true;
	auto queue = sceneMan.BuildRenderQueue(frustum, scene::RenderPassType::Opaque);
	sceneMan.RenderOpaquePass(queue, g_blinnPhongProgram);
	sceneMan.RenderTransparentPass(queue, g_blinnPhongProgram);

	// Pink wireframe overlay (cursor outline, no depth test)
	{
		auto& placeMode = ed::EditorApp::Get().GetPlaceMode();
		auto& mapMan = ed::EditorApp::Get().GetMapMan();
		if (placeMode.HasActiveCursor())
		{
			glm::vec3 cursorPos = placeMode.GetCursorWorldPos();
			glm::vec3 cursorEndPos = placeMode.GetCursorEndWorldPos();
			float spacing = mapMan.Tiles().GetSpacing();

			// For multi-select, compute center and size from the rectangle
			glm::vec3 modelPos = cursorPos;
			glm::vec3 modelScale = glm::vec3(spacing);
			if (glm::distance(cursorPos, cursorEndPos) > 1e-4f)
			{
				glm::vec3 minP = glm::min(cursorPos, cursorEndPos);
				glm::vec3 maxP = glm::max(cursorPos, cursorEndPos);
				modelPos = (minP + maxP) * 0.5f;
				modelScale = (maxP - minP) + glm::vec3(spacing);
			}

			gpu::cmd::BindShaderProgram(g_blinnPhongProgram);

			// Camera uniforms
			if (sceneMan.activeCamera && sceneMan.activeCamera->externalCamera)
			{
				auto* ec = sceneMan.activeCamera->externalCamera;
				gpu::program::SetUniform(g_blinnPhongProgram,
					gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_view"), ec->GetViewMatrix());
				gpu::program::SetUniform(g_blinnPhongProgram,
					gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_projection"),
					sceneMan.activeCamera->GetProjectionMatrix());
				gpu::program::SetUniform(g_blinnPhongProgram,
					gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_cameraPos"), ec->GetPosition());
			}

			// Model matrix
			glm::mat4 model = glm::translate(glm::mat4(1.0f), modelPos);
			model = glm::scale(model, modelScale);
			glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
			gpu::program::SetUniform(g_blinnPhongProgram,
				gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_model"), model);
			gpu::program::SetUniform(g_blinnPhongProgram,
				gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_normalMatrix"), normalMat);
			gpu::program::SetUniform(g_blinnPhongProgram,
				gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_isInstanced"), false);

			// Light UBO
			static auto overlayUBO = gpu::buffer::CreateBuffer(
				sizeof(LightBlockUBO),
				gpu::buffer::BufferStorageFlag::DynamicStorage,
				"overlay_light_ubo");
			{
				LightBlockUBO block{};
				block.lightCount = 1;
				auto& light = block.lights[0];
				light.positionOrDirection = glm::vec4(0.0f);
				light.color = glm::vec3(1.0f, 0.95f, 0.85f);
				light.intensity = 1.2f;
				light.spotDirection = glm::normalize(glm::vec3(-0.5f, 0.7f, -0.5f));
				light.type = 0;
				light.castShadow = 0;
				light.lightSpaceMatrix = glm::mat4(1.0f);
				gpu::buffer::UpdateData(overlayUBO, &block, sizeof(LightBlockUBO));
			}
			gpu::cmd::BindUniformBuffer(4, overlayUBO, 0, sizeof(LightBlockUBO));

			// Pink wireframe material
			gr::Material mat;
			mat.albedoColor = glm::vec3(1.0f, 0.3f, 0.8f);
			mat.ambientColor = glm::vec3(1.0f);
			mat.specularColor = glm::vec3(0.0f);
			mat.shininess = 1.0f;
			mat.opacity = 1.0f;
			mat.cullMode = gpu::CullMode::None;
			mat.Bind(g_blinnPhongProgram);

			gpu::program::SetUniform(g_blinnPhongProgram,
				gpu::program::GetUniformLocation(g_blinnPhongProgram, "u_receiveShadow"), false);

			// Override depth test: always pass
			gpu::DepthState ds;
			ds.depthTestEnable = true;
			ds.depthCompareOp = gpu::CompareOp::Always;
			ds.depthWriteEnable = false;
			gpu::cmd::SetState(ds);

			static auto overlayCube = std::make_shared<gr::Mesh>(gr::Mesh::CreateCubeWireframe());
			overlayCube->Bind();
			overlayCube->Draw();

			// Restore depth
			ds.depthCompareOp = gpu::CompareOp::Less;
			ds.depthWriteEnable = true;
			gpu::cmd::SetState(ds);
		}
	}

	gpu::cmd::EndDraw();
}
//=============================================================================
void EditorGameRenderUI()
{
	auto& app = ed::EditorApp::Get();

	// Main menu bar
	app.GetMenuBar().Draw();

	// Draw editor mode UI overlay
	if (auto* mode = app.GetEditorModePtr())
		mode->Draw();

	// Statistics panel
	auto& sceneMan = app.GetSceneManager();
	ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Text("Draw calls: %u", sceneMan.lastFrameStats.drawCalls);
		ImGui::Text("Instanced:  %u", sceneMan.lastFrameStats.instancedBatches);
		ImGui::Text("Culled:     %u", sceneMan.lastFrameStats.culledObjects);
	}
	ImGui::Text("Tiles:  %zu x %zu x %zu",
		app.GetMapMan().Tiles().GetWidth(),
		app.GetMapMan().Tiles().GetHeight(),
		app.GetMapMan().Tiles().GetLength());
	ImGui::Text("Textures: %d", app.GetMapMan().GetNumTextures());
	ImGui::Text("Models:   %d", app.GetMapMan().GetNumModels());
	ImGui::End();
}
//=============================================================================
// Entry point bridge
void EditorApp()
{
	ed::EditorApp::Get().Init(__argc, __argv);
}
//=============================================================================