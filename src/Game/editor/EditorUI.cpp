#include "stdafx.h"
#include "Editor.h"

namespace
{
	enum class FileDialogMode { NONE, OPEN, SAVE_AS };
	FileDialogMode g_fileDialogMode = FileDialogMode::NONE;
	char g_fileInputName[128] = "untitled";
	int  g_fileSelectedIdx    = -1;
	std::vector<std::string> g_fileList;

	void openFileDialog(FileDialogMode mode) noexcept
	{
		g_fileDialogMode = mode;
		g_fileSelectedIdx = -1;
		g_fileList = ListSavedMaps();
		memset(g_fileInputName, 0, sizeof(g_fileInputName));
		if (mode == FileDialogMode::SAVE_AS)
		{
			strncpy_s(g_fileInputName, sizeof(g_fileInputName),
				g_mapName.c_str(), _TRUNCATE);
		}
	}

	void closeFileDialog() noexcept
	{
		g_fileDialogMode = FileDialogMode::NONE;
		g_fileSelectedIdx = -1;
	}

	void showFileDialog() noexcept
	{
		if (g_fileDialogMode == FileDialogMode::NONE)
			return;

		const char* title = (g_fileDialogMode == FileDialogMode::OPEN)
			? "Open Map" : "Save Map As";
		ImGui::SetNextWindowSize(ImVec2(420, 360), ImGuiCond_Always);

		if (!ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_NoResize))
		{
			if (g_fileDialogMode != FileDialogMode::NONE)
				ImGui::OpenPopup(title);
			return;
		}

		// Map list
		ImGui::Text("Maps (%zu):", g_fileList.size());
		ImGui::BeginChild("map_list", ImVec2(0, 180), true);
		for (int i = 0; i < static_cast<int>(g_fileList.size()); ++i)
		{
			bool sel = (i == g_fileSelectedIdx);
			if (ImGui::Selectable(g_fileList[i].c_str(), &sel))
			{
				g_fileSelectedIdx = i;
				strncpy_s(g_fileInputName, sizeof(g_fileInputName),
					g_fileList[i].c_str(), _TRUNCATE);
			}
		}
		ImGui::EndChild();

		ImGui::Separator();

		// Name input
		ImGui::Text("Map name:");
		ImGui::InputText("##mapname", g_fileInputName, sizeof(g_fileInputName));

		ImGui::Separator();

		bool nameValid = (strlen(g_fileInputName) > 0);

		// Action button
		if (g_fileDialogMode == FileDialogMode::OPEN)
		{
			if (ImGui::Button("Open", ImVec2(120, 0)) && nameValid)
			{
				if (g_fileSelectedIdx >= 0 &&
					g_fileSelectedIdx < static_cast<int>(g_fileList.size()))
				{
					std::string path = "data/maps/" + g_fileList[g_fileSelectedIdx] + ".json";
					LoadMap(path);
				}
				closeFileDialog();
			}
		}
		else // SAVE_AS
		{
			if (ImGui::Button("Save", ImVec2(120, 0)) && nameValid)
			{
				SaveMapToPath(g_fileInputName);
				closeFileDialog();
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			closeFileDialog();
		}

		ImGui::EndPopup();
	}
}

// ---- GameRenderUI ----
void GameRenderUI()
{
	// Check for hotkey-requested dialogs
	if (g_requestOpenDialog)
	{
		g_requestOpenDialog = false;
		openFileDialog(FileDialogMode::OPEN);
	}
	if (g_requestSaveAsDialog)
	{
		g_requestSaveAsDialog = false;
		openFileDialog(FileDialogMode::SAVE_AS);
	}

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
			if (ImGui::MenuItem("New", "Ctrl+N"))
			{
				NewMap();
			}
			if (ImGui::MenuItem("Open", "Ctrl+O"))
			{
				openFileDialog(FileDialogMode::OPEN);
			}
			if (ImGui::MenuItem("Save", "Ctrl+S"))
			{
				if (!g_currentMapPath.empty())
					SaveMap(g_currentMapPath);
				else
					openFileDialog(FileDialogMode::SAVE_AS);
			}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
			{
				openFileDialog(FileDialogMode::SAVE_AS);
			}
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
				bool hasSolid = false;
				for (int ty = g_selTY; ty < g_selTY + g_selH && !hasSolid; ++ty)
					for (int tx = g_selTX; tx < g_selTX + g_selW && !hasSolid; ++tx)
						if (g_tileMap.InBounds(tx, ty) && g_tileMap.Get(tx, ty).spaceType == tile::TileSpaceType::SOLID)
							hasSolid = true;

				int refTX = (g_anchorTX >= 0 && g_tileMap.InBounds(g_anchorTX, g_anchorTY)) ? g_anchorTX : g_selTX;
				int refTY = (g_anchorTY >= 0 && g_tileMap.InBounds(g_anchorTX, g_anchorTY)) ? g_anchorTY : g_selTY;
				auto& refTile = g_tileMap.Get(refTX, refTY);

				if (hasSolid)
				{
					int srcTX = g_selTX, srcTY = g_selTY;
					bool found = false;
					for (int ty = g_selTY; ty < g_selTY + g_selH && !found; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW && !found; ++tx)
							if (g_tileMap.InBounds(tx, ty) && g_tileMap.Get(tx, ty).spaceType == tile::TileSpaceType::SOLID)
							{
								srcTX = tx; srcTY = ty; found = true;
							}
					auto& srcTile = g_tileMap.Get(srcTX, srcTY);
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tt = g_tileMap.Get(tx, ty);
							if (tt.spaceType == tile::TileSpaceType::EMPTY)
							{
								tt.spaceType   = tile::TileSpaceType::SOLID;
								tt.renderSolid = true;
								PropagateTileHeights(tt, tx, ty, &refTile);
							}
							tt.wallTex       = srcTile.wallTex;
							tt.wallBottomTex = srcTile.wallBottomTex;
							tt.floorTex      = srcTile.floorTex;
							tt.ceilTex       = srcTile.ceilTex;
							tt.wallAtlas       = srcTile.wallAtlas;
							tt.wallBottomAtlas = srcTile.wallBottomAtlas;
							tt.floorAtlas      = srcTile.floorAtlas;
							tt.ceilAtlas       = srcTile.ceilAtlas;
						}
				}
				else
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
							tt.wallAtlas       = static_cast<uint8_t>(g_brushWallAtlas);
							tt.wallBottomAtlas = static_cast<uint8_t>(g_brushWallBottomAtlas);
							tt.floorAtlas      = static_cast<uint8_t>(g_brushFloorAtlas);
							tt.ceilAtlas       = static_cast<uint8_t>(g_brushCeilAtlas);
							PropagateTileHeights(tt, tx, ty, &refTile);
						}
				}
				g_dirtyMesh = true;
			}
		}
					if (ImGui::MenuItem("Paint", "Shift+Enter"))
			{
				if (g_selTX >= 0)
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tt = g_tileMap.Get(tx, ty);
							if (tt.spaceType == tile::TileSpaceType::EMPTY)
							{
								tt.spaceType   = tile::TileSpaceType::SOLID;
								tt.renderSolid = true;
								PropagateTileHeights(tt, tx, ty);
							}
							tt.wallTex       = static_cast<uint8_t>(g_brushWallTex);
							tt.wallBottomTex = static_cast<uint8_t>(g_brushWallBottomTex);
							tt.floorTex      = static_cast<uint8_t>(g_brushFloorTex);
							tt.ceilTex       = static_cast<uint8_t>(g_brushCeilTex);
							tt.wallAtlas       = static_cast<uint8_t>(g_brushWallAtlas);
							tt.wallBottomAtlas = static_cast<uint8_t>(g_brushWallBottomAtlas);
							tt.floorAtlas      = static_cast<uint8_t>(g_brushFloorAtlas);
							tt.ceilAtlas       = static_cast<uint8_t>(g_brushCeilAtlas);
						}
					g_dirtyMesh = true;
				}
			}
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
							tt.wallAtlas = tt.wallBottomAtlas = tt.floorAtlas = tt.ceilAtlas = 0;
							tt.northTex = tt.southTex = tt.eastTex = tt.westTex = tile::TEX_NOT_SET;
							tt.northAtlas = tt.southAtlas = tt.eastAtlas = tt.westAtlas = 0;
							tt.bottomNorthTex = tt.bottomSouthTex = tt.bottomEastTex = tt.bottomWestTex = tile::TEX_NOT_SET;
							tt.bottomNorthAtlas = tt.bottomSouthAtlas = tt.bottomEastAtlas = tt.bottomWestAtlas = 0;
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
							tt.wallAtlas = tt.wallBottomAtlas = tt.floorAtlas = tt.ceilAtlas = 0;
							tt.northTex = tt.southTex = tt.eastTex = tt.westTex = tile::TEX_NOT_SET;
							tt.northAtlas = tt.southAtlas = tt.eastAtlas = tt.westAtlas = 0;
							tt.bottomNorthTex = tt.bottomSouthTex = tt.bottomEastTex = tt.bottomWestTex = tile::TEX_NOT_SET;
							tt.bottomNorthAtlas = tt.bottomSouthAtlas = tt.bottomEastAtlas = tt.bottomWestAtlas = 0;
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

// ---- Texture picker preview rows (T1/T2 atlas aware) ----
{
	constexpr float ATLAS_COLS = 16.0f;
	constexpr float ATLAS_ROWS = 8.0f;
	float thumbSize = 40.0f;
	uint32_t glId = gpu::texture::Handle(g_atlasTex);
	ImTextureID texId = (ImTextureID)(intptr_t)glId;

	auto tileUV = [](int globalIdx) -> std::pair<ImVec2, ImVec2>
	{
		float col = static_cast<float>(globalIdx % 16);
		float row = static_cast<float>(globalIdx / 16);
		float invCol = 1.0f / ATLAS_COLS;
		float invRow = 1.0f / ATLAS_ROWS;
		return { ImVec2(col * invCol, row * invRow), ImVec2((col + 1) * invCol, (row + 1) * invRow) };
	};

	auto drawPreview = [&](const char* label, int texVar, int atlasVar, int targetIdx)
	{
		int globalIdx = atlasVar * 64 + texVar;
		auto [uv0, uv1] = tileUV(globalIdx);
		ImGui::Image(ImTextureRef(texId), ImVec2(thumbSize, thumbSize), uv0, uv1);
		if (ImGui::IsItemClicked())
		{
			g_pickerTarget = targetIdx;
			g_selectedAtlas = atlasVar;
			g_showTexturePicker = true;
		}
		ImGui::SameLine();
		ImGui::Text("%s", label);
	};

	drawPreview("Upper Wall", g_brushWallTex, g_brushWallAtlas, 0);
	drawPreview("Lower Wall", g_brushWallBottomTex, g_brushWallBottomAtlas, 1);
	drawPreview("Ceiling", g_brushCeilTex, g_brushCeilAtlas, 2);
	drawPreview("Floor", g_brushFloorTex, g_brushFloorAtlas, 3);

	// Texture picker popup
	static const char* pickerTitles[] = {
		"Pick Upper Wall Texture",
		"Pick Lower Wall Texture",
		"Pick Ceiling Texture",
		"Pick Floor Texture"
	};
	static int* pickerTex[] = {
		&g_brushWallTex,
		&g_brushWallBottomTex,
		&g_brushCeilTex,
		&g_brushFloorTex
	};
	static int* pickerAtlas[] = {
		&g_brushWallAtlas,
		&g_brushWallBottomAtlas,
		&g_brushCeilAtlas,
		&g_brushFloorAtlas
	};

	if (g_showTexturePicker)
		ImGui::OpenPopup(pickerTitles[g_pickerTarget]);

	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Always);
	if (ImGui::BeginPopupModal(pickerTitles[g_pickerTarget], nullptr, ImGuiWindowFlags_NoResize))
	{
		ImGui::Combo("Atlas", &g_selectedAtlas, "T1\0T2\0");

		ImGui::BeginChild("grid", ImVec2(0, -(ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y)), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
		float cellSize = 60.0f;
		int cols = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / (cellSize + ImGui::GetStyle().ItemSpacing.x)));
		int totalTex = 64;
		int baseGlobal = g_selectedAtlas * 64;
		for (int ti = 0; ti < totalTex; ++ti)
		{
			int globalIdx = baseGlobal + ti;
			auto [uv0, uv1] = tileUV(globalIdx);

			ImGui::PushID(globalIdx);
			if (ImGui::ImageButton("##cell", ImTextureRef(texId), ImVec2(cellSize, cellSize), uv0, uv1))
			{
				*pickerTex[g_pickerTarget] = ti;
				*pickerAtlas[g_pickerTarget] = g_selectedAtlas;
				g_showTexturePicker = false;
				ImGui::CloseCurrentPopup();
			}
			ImGui::PopID();

			if ((ti + 1) % cols != 0)
				ImGui::SameLine();
		}
		ImGui::EndChild();

		if (ImGui::IsKeyPressed(ImGuiKey_Escape))
		{
			g_showTexturePicker = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::Separator();
}

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
						tt.wallAtlas       = static_cast<uint8_t>(g_brushWallAtlas);
						tt.wallBottomAtlas = static_cast<uint8_t>(g_brushWallBottomAtlas);
						tt.floorAtlas      = static_cast<uint8_t>(g_brushFloorAtlas);
						tt.ceilAtlas       = static_cast<uint8_t>(g_brushCeilAtlas);
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

	showFileDialog();

	ImGui::End();
}
