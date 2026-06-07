#include "stdafx.h"
#include "menu_bar.hpp"
#include "map_man/map_man.hpp"
#include "dialogs/dialogs.hpp"
#include <imgui/imgui.h>
#include <fstream>

//=============================================================================
ed::MenuBar::~MenuBar() = default;
//=============================================================================
ed::MenuBar::MenuBar(EditorApp::Settings& settings, MapMan& mapMan)
	: _settings(settings)
	, _mapMan(mapMan)
{}
//=============================================================================
void ed::MenuBar::Update()
{
	if (_messageTimer > 0.0f)
	{
		_messageTimer -= app::GetDeltaTime();
		if (_messageTimer <= 0.0f)
			_messageTimer = 0.0f;
	}
}
//=============================================================================
void ed::MenuBar::Draw()
{
	auto& app = EditorApp::Get();

	if (ImGui::BeginMainMenuBar())
	{
		drawMenuFile();
		drawMenuView();
		drawMenuConfig();
		drawMenuInfo();

		// Status message display
		if (!_statusMessage.empty() && _messageTimer > 0.0f)
		{
			float w = ImGui::GetContentRegionAvail().x;
			ImGui::SameLine(w - 300.0f);
			ImGui::TextUnformatted(_statusMessage.c_str());
		}

		ImGui::EndMainMenuBar();
	}

	// Draw active dialog
	if (_activeDialog)
	{
		bool open = _activeDialog->Draw();
		if (!open)
			_activeDialog.reset();
	}
}
//=============================================================================
void ed::MenuBar::drawMenuFile()
{
	auto& app = EditorApp::Get();

	if (ImGui::BeginMenu("MAP"))
	{
		if (ImGui::MenuItem("NEW"))
		{
			int w = 100, h = 5, l = 100;
			openDialog(std::make_unique<NewMapDialog>(w, h, l));
			// After dialog closes (handled above), use w/h/l
		}
		if (ImGui::MenuItem("OPEN"))
		{
			OpenOpenMapDialog();
		}
		if (ImGui::MenuItem("SAVE"))
		{
			SaveMap();
		}
		if (ImGui::MenuItem("SAVE AS"))
		{
			OpenSaveMapDialog();
		}
		if (ImGui::MenuItem("EXPORT"))
		{
			std::string outPath;
			bool separate = false;
			openDialog(std::make_unique<ExportDialog>(outPath, separate));
		}
		ImGui::Separator();
		if (ImGui::MenuItem("EXPAND GRID"))
		{
			int axis = 0, amount = 1;
			openDialog(std::make_unique<ExpandMapDialog>(axis, amount));
		}
		if (ImGui::MenuItem("SHRINK GRID"))
		{
			app.ShrinkMap();
		}
		ImGui::EndMenu();
	}
}
//=============================================================================
void ed::MenuBar::drawMenuView()
{
	auto& app = EditorApp::Get();

	if (ImGui::BeginMenu("VIEW"))
	{
		if (ImGui::MenuItem("MAP EDITOR", nullptr,
			app.GetEditorMode() == EditorApp::Mode::PLACE_TILE))
		{
			app.ChangeEditorMode(EditorApp::Mode::PLACE_TILE);
		}
		if (ImGui::MenuItem("TEXTURE PICKER", nullptr, false))
		{
			app.ChangeEditorMode(EditorApp::Mode::PICK_TEXTURE);
		}
		if (ImGui::MenuItem("SHAPE PICKER", nullptr, false))
		{
			app.ChangeEditorMode(EditorApp::Mode::PICK_SHAPE);
		}
		if (ImGui::MenuItem("ENTITY EDITOR", nullptr, false))
		{
			app.ChangeEditorMode(EditorApp::Mode::EDIT_ENT);
		}
		ImGui::Separator();
		if (ImGui::MenuItem("RESET CAMERA"))
		{
			app.ResetEditorCamera();
		}
		if (ImGui::MenuItem("TOGGLE PREVIEW"))
		{
			app.TogglePreviewing();
		}
		ImGui::EndMenu();
	}
}
//=============================================================================
void ed::MenuBar::drawMenuConfig()
{
	auto& app = EditorApp::Get();

	if (ImGui::BeginMenu("CONFIG"))
	{
		if (ImGui::MenuItem("ASSET PATHS"))
		{
			std::string texDir = _settings.texturesDir;
			std::string shapeDir = _settings.shapesDir;
			openDialog(std::make_unique<AssetPathDialog>(texDir, shapeDir));
		}
		if (ImGui::MenuItem("SETTINGS"))
		{
			size_t undoMax = _settings.undoMax;
			float sens = _settings.mouseSensitivity;
			bool cull = _settings.cullFaces;
			std::string hideRegex = _settings.assetHideRegex;
			openDialog(std::make_unique<SettingsDialog>(undoMax, sens, cull, hideRegex));
		}
		ImGui::EndMenu();
	}
}
//=============================================================================
void ed::MenuBar::drawMenuInfo()
{
	if (ImGui::BeginMenu("INFO"))
	{
		if (ImGui::MenuItem("ABOUT"))
		{
			openDialog(std::make_unique<AboutDialog>());
		}
		if (ImGui::MenuItem("KEYS / SHORTCUTS"))
		{
			openDialog(std::make_unique<ShortcutsDialog>());
		}
		if (ImGui::MenuItem("INSTRUCTIONS"))
		{
			openDialog(std::make_unique<InstructionsDialog>());
		}
		ImGui::EndMenu();
	}
}
//=============================================================================
void ed::MenuBar::OpenSaveMapDialog()
{
	std::string outPath;
	openDialog(std::make_unique<FileDialog>(outPath, "Save Map", ".te3", true));
}
//=============================================================================
void ed::MenuBar::OpenOpenMapDialog()
{
	std::string outPath;
	openDialog(std::make_unique<FileDialog>(outPath, "Open Map", ".te3", false));
}
//=============================================================================
void ed::MenuBar::SaveMap()
{
	auto& app = EditorApp::Get();
	if (app.GetLastSavedPath().empty())
	{
		OpenSaveMapDialog();
	}
	else
	{
		app.TrySaveMap(app.GetLastSavedPath());
	}
}
//=============================================================================
void ed::MenuBar::DisplayStatusMessage(std::string message, float durationSeconds, int priority)
{
	if (priority >= _messagePriority || _messageTimer <= 0.0f)
	{
		_statusMessage = std::move(message);
		_messageTimer = durationSeconds;
		_messagePriority = priority;
	}
}
//=============================================================================
void ed::MenuBar::openDialog(std::unique_ptr<Dialog> dialog)
{
	_activeDialog = std::move(dialog);
}
//=============================================================================