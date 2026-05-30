#include "stdafx.h"
#include "Editor.h"

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
int  g_selTX = -1, g_selTY = -1;
tile::FaceDir g_selFace = tile::FaceDir::COUNT;
int  g_selCorner = -1;
bool g_dirtyMesh = true;

gpu::program::ShaderProgramPtr g_debugProgram;
HeightEditMode g_heightEditMode = HeightEditMode::PLANE;
bool g_draggingCP = false;
int  g_dragCPType = 0;
int  g_dragCorner = 0;
float g_dragStartMouseY = 0;
float g_lastAppliedDy = 0;
float g_dragSlopes[4] = {};

float g_heightStep = 0.15f;
int   g_hoverCPIdx = -1;

int  g_brushWallTex   = 0;
int  g_brushFloorTex  = 1;
int  g_brushCeilTex   = 2;
bool g_brushSolid     = true;

int      g_mapWidth  = 16;
int      g_mapHeight = 16;
uint32_t g_genSeed   = 0;

int  g_hoverTX = -1, g_hoverTY = -1;
tile::FaceDir g_hoverFace = tile::FaceDir::COUNT;
int  g_prevHoverTX = -1, g_prevHoverTY = -1;
tile::FaceDir g_prevHoverFace = tile::FaceDir::COUNT;

// ---- Entry point ----
void GameAppTile()
{
	app::AppCreateInfo ci{};
	ci.init_cb        = GameInit;
	ci.close_cb       = GameClose;
	ci.update_cb      = GameUpdate;
	ci.fixedUpdate_cb = GameFixedUpdate;
	ci.render_cb      = GameRender;
	ci.renderUi_cb    = GameRenderUI;
	app::Run(ci);
}
