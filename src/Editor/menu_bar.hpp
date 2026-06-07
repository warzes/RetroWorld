#pragma once

#include <memory>
#include <string>
#include <functional>

#include "editor_app.hpp"

namespace ed
{
	class Dialog;
	class MapMan;

	class MenuBar final
	{
	public:
		MenuBar(EditorApp::Settings& settings, MapMan& mapMan);
		~MenuBar();

		void Update();
		void Draw();
		void OpenSaveMapDialog();
		void OpenOpenMapDialog();
		void SaveMap();
		void DisplayStatusMessage(std::string message, float durationSeconds, int priority);

	protected:
		EditorApp::Settings& _settings;
		MapMan& _mapMan;
		std::unique_ptr<Dialog> _activeDialog;
		std::string _statusMessage;
		float _messageTimer = 0.0f;
		int _messagePriority = 0;

	private:
		void drawMenuFile();
		void drawMenuView();
		void drawMenuConfig();
		void drawMenuInfo();
		void openDialog(std::unique_ptr<Dialog> dialog);
	};
} // namespace ed