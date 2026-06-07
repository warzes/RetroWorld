#pragma once

#include <memory>
#include <string>
#include "../editor_app.hpp"
#include "../ent.hpp"

namespace ed
{
	class FileDialog;

	class EntMode final : public EditorApp::ModeImpl
	{
	public:
		EntMode();
		~EntMode() override;
		void Update() override;
		void Draw() override;
		void OnEnter() override;
		void OnExit() override;

		const Ent& GetEnt() const { return _ent; }
		void SetEnt(const Ent& ent) { _ent = ent; }

	private:
		std::unique_ptr<FileDialog> _fileDialog;
		Ent _ent;
		char _texturePathBuffer[256];
		char _modelPathBuffer[256];
		char _newKeyBuffer[64];
		char _newValBuffer[256];
		std::map<std::string, std::string> _propEdits;
	};
} // namespace ed
