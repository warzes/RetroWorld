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
extern int  g_selTX, g_selTY;     // top-left of selection rect
extern int  g_selW, g_selH;       // selection dimensions (default 1×1)
extern int  g_anchorTX, g_anchorTY; // tile the user first clicked (persistent anchor for heights)
extern tile::FaceDir g_selFace;
extern int  g_selCorner;
extern bool g_dirtyMesh;
extern bool g_showCollider;

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
extern bool g_draggingSel;          // active tile-rect drag
extern int  g_dragStartTX, g_dragStartTY; // drag origin tile
extern int  g_dragCPType;  // CPType as int
extern int  g_dragCorner;  // for VERTEX mode corner index
extern int  g_dragVtxRefCount;           // number of refs sharing this vertex (0 = none)
extern int  g_dragVtxTX[4];              // tile X of each sharing tile
extern int  g_dragVtxTY[4];              // tile Y of each sharing tile
extern int  g_dragVtxCorner[4];          // corner index per ref (0-7)
extern float g_dragVtxInitSlope[4];      // initial slope per ref
extern float g_dragStartMouseY;
extern float g_lastAppliedDy;
extern float g_dragSlopes[4];
extern float g_dragCeilSlopes[4];

// Plane mode config
extern float g_heightStep;
extern int  g_hoverCPIdx;  // -1 = none

extern int  g_brushWallTex;
extern int  g_brushWallBottomTex;
extern int  g_brushFloorTex;
extern int  g_brushCeilTex;
extern int  g_brushNorthTex;
extern int  g_brushSouthTex;
extern int  g_brushEastTex;
extern int  g_brushWestTex;
extern int  g_brushBottomNorthTex;
extern int  g_brushBottomSouthTex;
extern int  g_brushBottomEastTex;
extern int  g_brushBottomWestTex;
extern bool g_brushSolid;

// Atlas selection in texture picker
extern int  g_selectedAtlas; // 0=T1, 1=T2

// Brush atlas IDs (corresponding to brush tex vars above)
extern int  g_brushWallAtlas;
extern int  g_brushWallBottomAtlas;
extern int  g_brushFloorAtlas;
extern int  g_brushCeilAtlas;

// Texture picker state
extern bool g_showTexturePicker;
extern int  g_pickerTarget; // 0=UpperWall, 1=LowerWall, 2=Ceiling, 3=Floor

extern int      g_mapWidth;
extern int      g_mapHeight;
extern uint32_t g_genSeed;

extern int  g_hoverTX, g_hoverTY;
extern tile::FaceDir g_hoverFace;
extern int  g_prevHoverTX, g_prevHoverTY;
extern tile::FaceDir g_prevHoverFace;
extern bool g_hoverDirty;
extern bool g_gameMode;
extern std::vector<glm::vec4> g_tileCleanColors;
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

// ---- Physics (Jolt) ----
class PhysicsSystem;
class PlayerController;

extern std::unique_ptr<PhysicsSystem>   g_physicsSystem;
extern std::unique_ptr<PlayerController> g_playerController;

// ---- Helpers ----
inline void PropagateTileHeights(tile::Tile& tt, int tx, int ty,
	const tile::Tile* fallback = nullptr) noexcept
{
	if (fallback)
	{
		tt.floorHeight = fallback->floorHeight;
		tt.ceilHeight  = fallback->ceilHeight;
	}

	auto match = [&](int nx, int ny)
	{
		if (!g_tileMap.InBounds(nx, ny)) return;
		auto& nb = g_tileMap.Get(nx, ny);
		if (nb.spaceType != tile::TileSpaceType::SOLID) return;

		if (nx == tx - 1) // West neighbor → match east corners
		{
			tt.slopeNW = nb.floorHeight + nb.slopeNE - tt.floorHeight;
			tt.slopeSW = nb.floorHeight + nb.slopeSE - tt.floorHeight;
			tt.ceilSlopeNW = nb.ceilHeight + nb.ceilSlopeNE - tt.ceilHeight;
			tt.ceilSlopeSW = nb.ceilHeight + nb.ceilSlopeSE - tt.ceilHeight;
		}
		else if (nx == tx + 1) // East neighbor → match west corners
		{
			tt.slopeNE = nb.floorHeight + nb.slopeNW - tt.floorHeight;
			tt.slopeSE = nb.floorHeight + nb.slopeSW - tt.floorHeight;
			tt.ceilSlopeNE = nb.ceilHeight + nb.ceilSlopeNW - tt.ceilHeight;
			tt.ceilSlopeSE = nb.ceilHeight + nb.ceilSlopeSW - tt.ceilHeight;
		}
		else if (ny == ty - 1) // North neighbor → match south corners
		{
			tt.slopeNW = nb.floorHeight + nb.slopeSW - tt.floorHeight;
			tt.slopeNE = nb.floorHeight + nb.slopeSE - tt.floorHeight;
			tt.ceilSlopeNW = nb.ceilHeight + nb.ceilSlopeSW - tt.ceilHeight;
			tt.ceilSlopeNE = nb.ceilHeight + nb.ceilSlopeSE - tt.ceilHeight;
		}
		else if (ny == ty + 1) // South neighbor → match north corners
		{
			tt.slopeSE = nb.floorHeight + nb.slopeNE - tt.floorHeight;
			tt.slopeSW = nb.floorHeight + nb.slopeNW - tt.floorHeight;
			tt.ceilSlopeSE = nb.ceilHeight + nb.ceilSlopeNE - tt.ceilHeight;
			tt.ceilSlopeSW = nb.ceilHeight + nb.ceilSlopeNW - tt.ceilHeight;
		}
	};

	match(tx - 1, ty);
	match(tx + 1, ty);
	match(tx, ty - 1);
	match(tx, ty + 1);
}

// ---- Functions ----
void RebuildTileMesh();
void UpdateHoverHighlight();
void PickTile(const glm::vec3& rayOrigin, const glm::vec3& rayDir);
void DrawDebugOverlay();
void DrawColliderOverlay();
void RebuildMapCollider();

bool GameInit();
void GameClose();
void GameUpdate();
void GameFixedUpdate();
void GameRender();
void GameRenderUI();

void GameAppTile();

// ---- File I/O ----
extern std::string g_currentMapPath;
extern std::string g_mapName;
extern bool g_requestOpenDialog;
extern bool g_requestSaveAsDialog;
void NewMap() noexcept;
bool SaveMap(std::string_view path) noexcept;
bool SaveMapToPath(std::string_view name) noexcept;
bool LoadMap(std::string_view path) noexcept;
std::vector<std::string> ListSavedMaps() noexcept;
