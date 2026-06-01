#include "stdafx.h"
#include "Editor.h"
#include "physics/PhysicsSystem.h"
#include "physics/PlayerController.h"

// ---- Global definitions ----
gpu::program::ShaderProgramPtr g_program;
gpu::program::ShaderProgramPtr g_depthShader;
gpu::program::ShaderProgramPtr g_pointDepthShader;

std::unique_ptr<scene::SceneManager> g_scene;
gr::Camera g_camera(glm::vec3(0, 10, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
input::MouseLook g_mouseLook;

tile::TileMap g_tileMap;
tile::TileMeshGen g_tileMeshCPU;
gr::Mesh g_tileMesh;
gr::Material g_tileMaterial;
gpu::texture::TexturePtr g_atlasTex;
gpu::texture::SamplerPtr g_atlasSampler;

scene::ModelNode* g_tileModelNode = nullptr;

EditMode g_editMode = EditMode::TILE;
EditorMode g_editorMode = EditorMode::TILE;
int  g_selTX = -1, g_selTY = -1;
int  g_selW = 1, g_selH = 1;
int  g_anchorTX = -1, g_anchorTY = -1;
tile::FaceDir g_selFace = tile::FaceDir::COUNT;
int  g_selCorner = -1;
bool g_dirtyMesh = true;
bool g_showCollider = false;

gpu::program::ShaderProgramPtr g_debugProgram;
HeightEditMode g_heightEditMode = HeightEditMode::PLANE;
bool g_draggingCP = false;
bool g_draggingSel = false;
int  g_dragStartTX = 0, g_dragStartTY = 0;
int  g_dragCPType = 0;
int  g_dragCorner = 0;
int  g_dragVtxRefCount = 0;
int  g_dragVtxTX[4] = {};
int  g_dragVtxTY[4] = {};
int  g_dragVtxCorner[4] = {};
float g_dragVtxInitSlope[4] = {};
float g_dragStartMouseY = 0;
float g_lastAppliedDy = 0;
float g_dragSlopes[4] = {};
float g_dragCeilSlopes[4] = {};

float g_heightStep = 0.1f;
int   g_hoverCPIdx = -1;

int  g_brushWallTex       = 0;
int  g_brushWallBottomTex = 0;
int  g_brushFloorTex      = 1;
int  g_brushCeilTex       = 2;
int  g_brushNorthTex      = -1;
int  g_brushSouthTex      = -1;
int  g_brushEastTex       = -1;
int  g_brushWestTex       = -1;
int  g_brushBottomNorthTex = -1;
int  g_brushBottomSouthTex = -1;
int  g_brushBottomEastTex  = -1;
int  g_brushBottomWestTex  = -1;
bool g_brushSolid         = true;

int  g_selectedAtlas       = 0;

int  g_brushWallAtlas       = 0;
int  g_brushWallBottomAtlas = 0;
int  g_brushFloorAtlas      = 0;
int  g_brushCeilAtlas       = 0;

bool g_showTexturePicker   = false;
int  g_pickerTarget        = 0;

// Decoration state
std::vector<decorations::Instance> g_decorations;
int  g_selectedDecoration    = -1;
bool g_showDecorationPicker  = false;
bool g_showDecorationInspector = true;
bool g_decorationSnapToTile  = true;
std::string g_decorationPickerFolder;
std::string g_decorationPickerModel;
int  g_decorationPickerModelIdx = -1;
scene::ModelNode* g_decorationPreviewNode = nullptr;

int      g_mapWidth  = 20;
int      g_mapHeight = 20;
uint32_t g_genSeed   = 0;

int  g_hoverTX = -1, g_hoverTY = -1;
tile::FaceDir g_hoverFace = tile::FaceDir::COUNT;
int  g_prevHoverTX = -1, g_prevHoverTY = -1;
tile::FaceDir g_prevHoverFace = tile::FaceDir::COUNT;
bool g_hoverDirty = false;
bool g_gameMode = false;
std::vector<glm::vec4> g_tileCleanColors;

std::string g_currentMapPath;
std::string g_mapName = "untitled";
bool g_requestOpenDialog   = false;
bool g_requestSaveAsDialog = false;

// Physics
std::unique_ptr<PhysicsSystem>   g_physicsSystem;
std::unique_ptr<PlayerController> g_playerController;

// ---- Entry point ----
void GameAppTile()
{
	app::AppCreateInfo ci{};
	ci.window.adaptiveVsync = false;
	ci.init_cb        = GameInit;
	ci.close_cb       = GameClose;
	ci.update_cb      = GameUpdate;
	ci.fixedUpdate_cb = GameFixedUpdate;
	ci.render_cb      = GameRender;
	ci.renderUi_cb    = GameRenderUI;
	app::Run(ci);
}
