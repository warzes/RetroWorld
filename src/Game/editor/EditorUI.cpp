#include "stdafx.h"
#include "Editor.h"

// ---- GameRenderUI ----
void GameRenderUI()
{
	// Main Menu Bar (Delver Engine style)
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N")) {}
			if (ImGui::BeginMenu("Open Recent"))
			{
				ImGui::Text("(empty)");
				ImGui::EndMenu();
			}
			ImGui::Separator();
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
			if (ImGui::MenuItem("Carve", "Enter")) {}
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
						}
					g_dirtyMesh = true;
				}
			}
			if (ImGui::MenuItem("Deselect", "Escape")) {}
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
		if (ImGui::Button(">")) {}

		ImGui::EndMainMenuBar();
	}

	// Editor panel
	ImGui::Begin("Tile Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("WASD=move  RMB=look  1/2/3=Mode  R=Regen");
	ImGui::Separator();

	int mode = static_cast<int>(g_editMode);
	ImGui::RadioButton("Tile",   &mode, 0); ImGui::SameLine();
	ImGui::RadioButton("Face",   &mode, 1); ImGui::SameLine();
	ImGui::RadioButton("Vertex", &mode, 2);
	g_editMode = static_cast<EditMode>(mode);

	ImGui::Separator();

	if (g_editMode == EditMode::TILE)
	{
		ImGui::Text("Height Edit Mode: %s  [V]", 
			g_heightEditMode == HeightEditMode::PLANE ? "Plane" : "Vertex");
		ImGui::Separator();

		ImGui::Text("Brush:");
		ImGui::Checkbox("Solid", &g_brushSolid);
		ImGui::SliderInt("Wall Tex",  &g_brushWallTex,  0, 63);
		ImGui::SliderInt("Floor Tex", &g_brushFloorTex, 0, 63);
		ImGui::SliderInt("Ceil Tex",  &g_brushCeilTex,  0, 63);

		ImGui::Separator();
		ImGui::Text("Click tile to select.");
		ImGui::SliderFloat("Height Step", &g_heightStep, 0.01f, 1.0f, "%.2f");
		ImGui::Text("Drag orange/blue markers to adjust height.");
		if (g_heightEditMode == HeightEditMode::VERTEX)
			ImGui::Text("Drag green/orange corner markers to adjust floor/ceil.");

		if (g_selTX >= 0)
		{
			ImGui::Text("Selected: (%d, %d)  size %dx%d", g_selTX, g_selTY, g_selW, g_selH);
			auto& t = g_tileMap.Get(g_selTX, g_selTY);

			int st = static_cast<int>(t.spaceType);
			ImGui::RadioButton("Empty", &st, 0); ImGui::SameLine();
			ImGui::RadioButton("Solid", &st, 1);
			{
				auto newType = static_cast<tile::TileSpaceType>(st);
				bool newSolid = (st == 1);
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						if (tt.spaceType != newType || tt.renderSolid != newSolid)
						{
							tt.spaceType   = newType;
							tt.renderSolid = newSolid;
							g_dirtyMesh = true;
						}
					}
			}

			int wt = t.wallTex, ft = t.floorTex, ct = t.ceilTex;
			if (ImGui::SliderInt("Wall",  &wt, 0, 63))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						g_tileMap.Get(tx, ty).wallTex = static_cast<uint8_t>(wt);
					}
				g_dirtyMesh = true;
			}
			if (ImGui::SliderInt("Floor", &ft, 0, 63))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						g_tileMap.Get(tx, ty).floorTex = static_cast<uint8_t>(ft);
					}
				g_dirtyMesh = true;
			}
			if (ImGui::SliderInt("Ceil",  &ct, 0, 63))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						g_tileMap.Get(tx, ty).ceilTex = static_cast<uint8_t>(ct);
					}
				g_dirtyMesh = true;
			}

			ImGui::Text("Floor: %.2f", t.floorHeight);
			ImGui::Text("Ceil:  %.2f", t.ceilHeight);
			ImGui::Text("Slopes: NW=%.2f NE=%.2f SE=%.2f SW=%.2f",
				t.slopeNW, t.slopeNE, t.slopeSE, t.slopeSW);

			if (ImGui::Button("Remove Tile"))
			{
				for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
					{
						if (!g_tileMap.InBounds(tx, ty)) continue;
						auto& tt = g_tileMap.Get(tx, ty);
						tt.spaceType   = tile::TileSpaceType::EMPTY;
						tt.renderSolid = false;
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
						tt.wallTex     = static_cast<uint8_t>(g_brushWallTex);
						tt.floorTex    = static_cast<uint8_t>(g_brushFloorTex);
						tt.ceilTex     = static_cast<uint8_t>(g_brushCeilTex);
					}
				g_dirtyMesh = true;
			}
		}
	}

	if (g_editMode == EditMode::FACE)
	{
		if (g_selTX >= 0)
		{
			auto& t = g_tileMap.Get(g_selTX, g_selTY);
			ImGui::Text("Tile: (%d, %d)  size %dx%d", g_selTX, g_selTY, g_selW, g_selH);
			ImGui::Text("Face: %s", g_selFace < tile::FaceDir::COUNT ?
				tile::FaceNames[static_cast<int>(g_selFace)] : "none");

			if (g_selFace == tile::FaceDir::FLOOR)
			{
				int ft = t.floorTex;
				if (ImGui::SliderInt("Floor Texture", &ft, 0, 63))
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							g_tileMap.Get(tx, ty).floorTex = static_cast<uint8_t>(ft);
						}
					g_dirtyMesh = true;
				}
			}
			else if (g_selFace == tile::FaceDir::CEILING)
			{
				int ct = t.ceilTex;
				if (ImGui::SliderInt("Ceil Texture", &ct, 0, 63))
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							g_tileMap.Get(tx, ty).ceilTex = static_cast<uint8_t>(ct);
						}
					g_dirtyMesh = true;
				}
			}
			else
			{
				int wt = t.wallTex;
				if (ImGui::SliderInt("Wall Texture", &wt, 0, 63))
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							g_tileMap.Get(tx, ty).wallTex = static_cast<uint8_t>(wt);
						}
					g_dirtyMesh = true;
				}
			}
		}
		else ImGui::Text("Click on a face to select.");
	}

	if (g_editMode == EditMode::VERTEX)
	{
		const char* cornerNames[] = { "NW", "NE", "SE", "SW" };
		if (g_selTX >= 0 && g_selCorner >= 0)
		{
			ImGui::Text("Tile: (%d, %d)", g_selTX, g_selTY);
			ImGui::Text("Corner: %s", cornerNames[g_selCorner]);

			auto& t = g_tileMap.Get(g_selTX, g_selTY);
			float* fSlope = nullptr;
			float* cSlope = nullptr;
			switch (g_selCorner)
			{
				case 0: fSlope = &t.slopeNW; cSlope = &t.ceilSlopeNW; break;
				case 1: fSlope = &t.slopeNE; cSlope = &t.ceilSlopeNE; break;
				case 2: fSlope = &t.slopeSE; cSlope = &t.ceilSlopeSE; break;
				case 3: fSlope = &t.slopeSW; cSlope = &t.ceilSlopeSW; break;
			}
			if (fSlope && cSlope)
			{
				float fVal = *fSlope;
				if (ImGui::SliderFloat("Floor Offset", &fVal, -1.0f, 1.0f))
				{
					*fSlope = fVal;
					clampFloorVertex(*fSlope, t.floorHeight, *cSlope, t.ceilHeight);
					g_dirtyMesh = true;
				}
				float cVal = *cSlope;
				if (ImGui::SliderFloat("Ceil Offset", &cVal, -1.0f, 1.0f))
				{
					*cSlope = cVal;
					clampCeilVertex(*cSlope, t.ceilHeight, *fSlope, t.floorHeight);
					g_dirtyMesh = true;
				}
			}
			ImGui::Text("Scroll to adjust floor; drag corner markers (TILE+V).");
		}
		else ImGui::Text("Click near a corner to select.");
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
