#pragma once

#include <algorithm>
#include "tile/TileTypes.h"
#include "tile/TileMap.h"
#include "tile/TileMeshGen.h"
#include "tile/TileAtlas.h"
#include "tile/TilePicking.h"

#include <gr_mesh.h>
#include <gr_material.h>
#include <gpu_texture.h>
#include <gpu_program.h>
#include <sc_sceneManager.h>
#include <gr_camera.h>
#include <app_mouseLook.h>

// ---- Globals ----
extern gpu::program::ShaderProgramPtr g_program;
extern gpu::program::ShaderProgramPtr g_depthShader;
extern gpu::program::ShaderProgramPtr g_pointDepthShader;

extern std::unique_ptr<scene::SceneManager> g_scene;
extern gr::Camera g_camera;
extern input::MouseLook g_mouseLook;

extern tile::TileMap g_tileMap;
extern tile::TileMeshGen g_tileMeshCPU;
extern gr::Mesh g_tileMesh;
extern gr::Material g_tileMaterial;
extern gpu::texture::TexturePtr g_atlasTex;
extern gpu::texture::SamplerPtr g_atlasSampler;

extern scene::ModelNode* g_tileModelNode;

// ---- Editor state ----
enum class EditMode { TILE, FACE, VERTEX };

extern EditMode g_editMode;
extern int  g_selTX, g_selTY;
extern tile::FaceDir g_selFace;
extern int  g_selCorner;
extern bool g_dirtyMesh;

// Height edit sub-mode for TILE mode
enum class HeightEditMode { PLANE, VERTEX };

// Control point types for Plane mode (10 markers)
enum class CPType : int {
	FloorCenter = 0,
	CeilCenter,
	FloorNorth, FloorSouth, FloorWest, FloorEast,
	CeilNorth, CeilSouth, CeilWest, CeilEast,
	Corner,  // VERTEX mode corner
	COUNT
};

extern gpu::program::ShaderProgramPtr g_debugProgram;
extern HeightEditMode g_heightEditMode;

extern bool g_draggingCP;
extern int  g_dragCPType;  // CPType as int
extern int  g_dragCorner;  // for VERTEX mode corner index
extern float g_dragStartMouseY;
extern float g_lastAppliedDy;
extern float g_dragSlopes[4];
extern float g_dragCeilSlopes[4];

// Plane mode config
extern float g_heightStep;
extern int  g_hoverCPIdx;  // -1 = none

extern int  g_brushWallTex;
extern int  g_brushFloorTex;
extern int  g_brushCeilTex;
extern bool g_brushSolid;

extern int      g_mapWidth;
extern int      g_mapHeight;
extern uint32_t g_genSeed;

extern int  g_hoverTX, g_hoverTY;
extern tile::FaceDir g_hoverFace;
extern int  g_prevHoverTX, g_prevHoverTY;
extern tile::FaceDir g_prevHoverFace;
inline const glm::vec4 HOVER_PINK{1.5f, 0.3f, 0.8f, 1.0f};

// Height clamping helpers (used by EditorApp.cpp and EditorUI.cpp)
constexpr float MIN_GAP = 0.02f;
inline void clampFloorVertex(float& fSlope, float fh, float cSlope, float ch) noexcept
{
	float maxY = (ch + cSlope) - MIN_GAP;
	fSlope = std::min(fSlope, maxY - fh);
}
inline void clampCeilVertex(float& cSlope, float ch, float fSlope, float fh) noexcept
{
	float minY = (fh + fSlope) + MIN_GAP;
	cSlope = std::max(cSlope, minY - ch);
}

// ---- Functions ----
void RebuildTileMesh();
void PickTile(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
void DrawDebugOverlay();

bool GameInit();
void GameClose();
void GameUpdate();
void GameFixedUpdate();
void GameRender();
void GameRenderUI();

void GameAppTile();
