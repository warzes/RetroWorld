#include "stdafx.h"
#include "Editor.h"

// ---- GameRenderUI ----
void GameRenderUI()
{
	// ---- Game mode camera info ----
	if (g_gameMode)
	{
		ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.5f);
		if (ImGui::Begin("Camera", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings))
		{
			auto pos = g_camera.GetPosition();
			ImGui::Text("Pos: %.2f %.2f %.2f", pos.x, pos.y, pos.z);
			ImGui::Text("Tab / > - Editor");
			if (ImGui::Button("Back to Editor"))
			{
				g_gameMode = false;
			}
			ImGui::Checkbox("Show Collider", &g_showCollider);
		}
		ImGui::End();
		return;
	}

	// ---- Editor UI ----
	// Main Menu Bar (Delver Engine style)
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N")) {}
			if (ImGui::MenuItem("Save", "Ctrl+S")) {}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Copy", "Ctrl+C")) {}
			if (ImGui::MenuItem("Paste", "Ctrl+V")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tile"))
		{
			if (ImGui::MenuItem("Carve", "Enter"))
			{
				if (g_selTX >= 0)
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tt = g_tileMap.Get(tx, ty);
							if (tt.spaceType == tile::TileSpaceType::SOLID) continue;
							tt.spaceType   = tile::TileSpaceType::SOLID;
							tt.renderSolid = true;
							tt.wallTex       = static_cast<uint8_t>(g_brushWallTex);
							tt.wallBottomTex = static_cast<uint8_t>(g_brushWallBottomTex);
							tt.floorTex      = static_cast<uint8_t>(g_brushFloorTex);
							tt.ceilTex       = static_cast<uint8_t>(g_brushCeilTex);
							tt.northTex       = (g_brushNorthTex      < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushNorthTex);
							tt.southTex       = (g_brushSouthTex      < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushSouthTex);
							tt.eastTex        = (g_brushEastTex       < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushEastTex);
							tt.westTex        = (g_brushWestTex       < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushWestTex);
							tt.bottomNorthTex = (g_brushBottomNorthTex < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomNorthTex);
							tt.bottomSouthTex = (g_brushBottomSouthTex < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomSouthTex);
							tt.bottomEastTex  = (g_brushBottomEastTex  < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomEastTex);
							tt.bottomWestTex  = (g_brushBottomWestTex  < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomWestTex);
						}
					g_dirtyMesh = true;
				}
			}
			if (ImGui::MenuItem("Paint", "Shift+Enter")) {}
			if (ImGui::MenuItem("Delete", "Del"))
			{
				if (g_selTX >= 0) {
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tt = g_tileMap.Get(tx, ty);
							tt.spaceType   = tile::TileSpaceType::EMPTY;
							tt.renderSolid = false;
							tt.floorHeight = -0.5f;
							tt.ceilHeight  =  0.5f;
							tt.slopeNW = tt.slopeNE = tt.slopeSE = tt.slopeSW     = 0.0f;
							tt.ceilSlopeNW = tt.ceilSlopeNE = tt.ceilSlopeSE = tt.ceilSlopeSW = 0.0f;
							tt.wallBottomTex = 0;
							tt.northTex = tt.southTex = tt.eastTex = tt.westTex = tile::TEX_NOT_SET;
							tt.bottomNorthTex = tt.bottomSouthTex = tt.bottomEastTex = tt.bottomWestTex = tile::TEX_NOT_SET;
						}
					g_dirtyMesh = true;
				}
			}
			if (ImGui::MenuItem("Deselect", "Escape"))
			{
				g_selTX = g_selTY = -1;
				g_anchorTX = g_anchorTY = -1;
				g_selW = 1; g_selH = 1;
				g_selFace = tile::FaceDir::COUNT;
				g_selCorner = -1;
				g_draggingCP = false;
				g_draggingSel = false;
				g_hoverCPIdx = -1;
				g_dragVtxRefCount = 0;
			}
			if (ImGui::MenuItem("Reset"))
			{
				if (g_selTX >= 0) {
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tt = g_tileMap.Get(tx, ty);
							tt.floorHeight = -0.5f;
							tt.ceilHeight  =  0.5f;
							tt.slopeNW = tt.slopeNE = tt.slopeSE = tt.slopeSW     = 0.0f;
							tt.ceilSlopeNW = tt.ceilSlopeNE = tt.ceilSlopeSE = tt.ceilSlopeSW = 0.0f;
							tt.wallBottomTex = 0;
							tt.northTex = tt.southTex = tt.eastTex = tt.westTex = tile::TEX_NOT_SET;
							tt.bottomNorthTex = tt.bottomSouthTex = tt.bottomEastTex = tt.bottomWestTex = tile::TEX_NOT_SET;
						}
					g_dirtyMesh = true;
				}
			}
			ImGui::Separator();

			if (ImGui::BeginMenu("Height Edit Mode"))
			{
				bool isPlane = (g_heightEditMode == HeightEditMode::PLANE);
				if (ImGui::MenuItem("Plane", nullptr, &isPlane))
					g_heightEditMode = HeightEditMode::PLANE;
				bool isVertex = (g_heightEditMode == HeightEditMode::VERTEX);
				if (ImGui::MenuItem("Vertex", nullptr, &isVertex))
					g_heightEditMode = HeightEditMode::VERTEX;
				if (ImGui::MenuItem("Toggle", "V"))
					g_heightEditMode = (g_heightEditMode == HeightEditMode::PLANE)
						? HeightEditMode::VERTEX : HeightEditMode::PLANE;
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Raise/Lower"))
			{
				if (ImGui::MenuItem("Raise Floor", "3")) {}
				if (ImGui::MenuItem("Lower Floor", "Shift+3")) {}
				if (ImGui::MenuItem("Raise Ceiling", "4")) {}
				if (ImGui::MenuItem("Lower Ceiling", "Shift+4")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Move"))
			{
				if (ImGui::MenuItem("Move North", "Shift+Up")) {}
				if (ImGui::MenuItem("Move South", "Shift+Down")) {}
				if (ImGui::MenuItem("Move East", "Shift+Left")) {}
				if (ImGui::MenuItem("Move West", "Shift+Right")) {}
				if (ImGui::MenuItem("Move Up", "Shift+E")) {}
				if (ImGui::MenuItem("Move Down", "Shift+Q")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Rotate Wall Angle", "U")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Flatten"))
			{
				if (ImGui::MenuItem("Floor", "F")) {}
				if (ImGui::MenuItem("Ceiling", "Shift+F")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Pick Textures", "G")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Rotate Texture"))
			{
				if (ImGui::MenuItem("Floor", "T")) {}
				if (ImGui::MenuItem("Ceiling", "Shift+T")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Surface"))
			{
				if (ImGui::MenuItem("Paint Surface Texture", "1")) {}
				if (ImGui::MenuItem("Grab Surface Texture", "2")) {}
				if (ImGui::MenuItem("Pick Surface Texture", "Shift+2")) {}
				if (ImGui::MenuItem("Flood Fill Surface Texture", "Shift+1")) {}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Entity"))
		{
			if (ImGui::MenuItem("Delete", "Del")) {}
			if (ImGui::MenuItem("Deselect", "Escape")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Move"))
			{
				if (ImGui::MenuItem("Constrain to X-axis", "X")) {}
				if (ImGui::MenuItem("Constrain to Y-axis", "Y")) {}
				if (ImGui::MenuItem("Constrain to Z-axis", "Z")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Rotate", "R")) {}

			if (ImGui::BeginMenu("Turn"))
			{
				if (ImGui::MenuItem("Clockwise", "Ctrl+Left")) {}
				if (ImGui::MenuItem("Counter-clockwise", "Ctrl+Right")) {}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Toggle Simulation", "B")) {}
			if (ImGui::MenuItem("Toggle Gizmos")) {}
			if (ImGui::MenuItem("Toggle Lights", "L")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("View Selected", "Space")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Level"))
		{
			if (ImGui::MenuItem("Play From Camera", "P")) {}
			if (ImGui::MenuItem("Play From Start", "Shift+P")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Rotate Level"))
			{
				if (ImGui::MenuItem("Clockwise")) {}
				if (ImGui::MenuItem("Counter-clockwise")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Resize Level")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Set Theme")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Generate Room")) {}
			if (ImGui::MenuItem("Generate Level")) {}
			ImGui::EndMenu();
		}

		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		if (ImGui::Button(">"))
		{
			g_gameMode = true;
			g_selTX = g_selTY = -1;
			g_anchorTX = g_anchorTY = -1;
			g_selW = 1; g_selH = 1;
			g_selFace = tile::FaceDir::COUNT;
			g_selCorner = -1;
			g_draggingCP = false;
			g_draggingSel = false;
			g_hoverCPIdx = -1;
			g_dragVtxRefCount = 0;
		}

		ImGui::EndMainMenuBar();
	}

	// Editor panel
	ImGui::Begin("Tile Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	{
		ImGui::Text("Height Edit Mode: %s  [V]",  g_heightEditMode == HeightEditMode::PLANE ? "Plane" : "Vertex");
		ImGui::Separator();

		if (g_selTX >= 0)
		{
			ImGui::Text("Selected: (%d, %d)  size %dx%d", g_selTX, g_selTY, g_selW, g_selH);
			auto& t = g_tileMap.Get(g_selTX, g_selTY);

			{
				int st = static_cast<int>(t.spaceType);
				ImGui::RadioButton("Empty", &st, 0); ImGui::SameLine();
				ImGui::RadioButton("Solid", &st, 1);
				if (st != static_cast<int>(t.spaceType))
				{
					t.spaceType   = static_cast<tile::TileSpaceType>(st);
					t.renderSolid = (st == 1);
					g_dirtyMesh   = true;
				}
			}

			int wt = t.wallTex, ft = t.floorTex, ct = t.ceilTex;
			if (ImGui::SliderInt("Wall",  &wt, 0, 63))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						if (tt.spaceType != tile::TileSpaceType::SOLID) continue;
						tt.wallTex = static_cast<uint8_t>(wt);
					}
				g_dirtyMesh = true;
			}
			{
				int wbt = t.wallBottomTex;
				if (ImGui::SliderInt("Wall Bottom", &wbt, 0, 63))
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tt = g_tileMap.Get(tx, ty);
							if (tt.spaceType != tile::TileSpaceType::SOLID) continue;
							tt.wallBottomTex = static_cast<uint8_t>(wbt);
						}
					g_dirtyMesh = true;
				}
			}
			if (ImGui::CollapsingHeader("Per-Dir Overrides"))
			{
				auto dirSlider = [&](const char* label, uint8_t& field)
				{
					int val = (field == tile::TEX_NOT_SET) ? -1 : static_cast<int>(field);
					if (ImGui::SliderInt(label, &val, -1, 63))
					{
						for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
							for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
							{
								if (!g_tileMap.InBounds(tx, ty)) continue;
								auto& tt = g_tileMap.Get(tx, ty);
								if (tt.spaceType != tile::TileSpaceType::SOLID) continue;
								field = (val < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(val);
							}
						g_dirtyMesh = true;
					}
				};
				ImGui::Text("Upper (-1 = use Wall Tex)");
				dirSlider("North", t.northTex);
				dirSlider("South", t.southTex);
				dirSlider("East",  t.eastTex);
				dirSlider("West",  t.westTex);
				ImGui::Separator();
				ImGui::Text("Lower (-1 = Wall Bottom -> Wall Tex)");
				dirSlider("Bottom North", t.bottomNorthTex);
				dirSlider("Bottom South", t.bottomSouthTex);
				dirSlider("Bottom East",  t.bottomEastTex);
				dirSlider("Bottom West",  t.bottomWestTex);
			}
			if (ImGui::SliderInt("Floor", &ft, 0, 63))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						if (tt.spaceType != tile::TileSpaceType::SOLID) continue;
						tt.floorTex = static_cast<uint8_t>(ft);
					}
				g_dirtyMesh = true;
			}
			if (ImGui::SliderInt("Ceil",  &ct, 0, 63))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						if (tt.spaceType != tile::TileSpaceType::SOLID) continue;
						tt.ceilTex = static_cast<uint8_t>(ct);
					}
				g_dirtyMesh = true;
			}

			if (t.spaceType == tile::TileSpaceType::SOLID)
			{
				ImGui::Text("Floor: %.2f", t.floorHeight);
				ImGui::Text("Ceil:  %.2f", t.ceilHeight);
				ImGui::Text("Slopes: NW=%.2f NE=%.2f SE=%.2f SW=%.2f",
					t.slopeNW, t.slopeNE, t.slopeSE, t.slopeSW);
			}

			if (ImGui::Button("Remove Tile"))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						tt.spaceType   = tile::TileSpaceType::EMPTY;
						tt.renderSolid = false;
						tt.floorHeight = -0.5f;
						tt.ceilHeight  =  0.5f;
						tt.slopeNW = tt.slopeNE = tt.slopeSE = tt.slopeSW     = 0.0f;
						tt.ceilSlopeNW = tt.ceilSlopeNE = tt.ceilSlopeSE = tt.ceilSlopeSW = 0.0f;
						tt.wallBottomTex = 0;
						tt.northTex = tt.southTex = tt.eastTex = tt.westTex = tile::TEX_NOT_SET;
						tt.bottomNorthTex = tt.bottomSouthTex = tt.bottomEastTex = tt.bottomWestTex = tile::TEX_NOT_SET;
					}
				g_dirtyMesh = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Place Tile"))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						tt.spaceType   = tile::TileSpaceType::SOLID;
						tt.renderSolid = true;
						tt.wallTex       = static_cast<uint8_t>(g_brushWallTex);
						tt.wallBottomTex = static_cast<uint8_t>(g_brushWallBottomTex);
						tt.floorTex      = static_cast<uint8_t>(g_brushFloorTex);
						tt.ceilTex       = static_cast<uint8_t>(g_brushCeilTex);
						tt.northTex       = (g_brushNorthTex      < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushNorthTex);
						tt.southTex       = (g_brushSouthTex      < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushSouthTex);
						tt.eastTex        = (g_brushEastTex       < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushEastTex);
						tt.westTex        = (g_brushWestTex       < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushWestTex);
						tt.bottomNorthTex = (g_brushBottomNorthTex < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomNorthTex);
						tt.bottomSouthTex = (g_brushBottomSouthTex < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomSouthTex);
						tt.bottomEastTex  = (g_brushBottomEastTex  < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomEastTex);
						tt.bottomWestTex  = (g_brushBottomWestTex  < 0) ? tile::TEX_NOT_SET : static_cast<uint8_t>(g_brushBottomWestTex);
					}
				g_dirtyMesh = true;
			}
		}
	}

	ImGui::Separator();
	ImGui::Text("Map: %d x %d", g_mapWidth, g_mapHeight);
	if (ImGui::Button("Regenerate"))
	{
		g_tileMap.GenerateRandom(++g_genSeed);
		g_selTX = g_selTY = g_selCorner = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_dirtyMesh = true;
	}

	ImGui::Separator();
	ImGui::Text("Verts: %zu", g_tileMeshCPU.positions.size());
	ImGui::Text("Tris:  %zu", g_tileMeshCPU.indices.size() / 3);
	ImGui::Text("Draw calls: %u", g_scene->lastFrameStats.drawCalls);

	ImGui::End();
}
