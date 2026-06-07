#pragma once

#include <memory>
#include <string>
#include <functional>
#include <filesystem>

namespace ed
{
	class Dialog
	{
	public:
		virtual ~Dialog() = default;
		// Returns false when dialog should close
		virtual bool Draw() = 0;
	};

	class NewMapDialog final : public Dialog
	{
	public:
		NewMapDialog(int& outWidth, int& outHeight, int& outLength);
		bool Draw() override;
	private:
		int _width, _height, _length;
		int& _outWidth, & _outHeight, & _outLength;
		bool _confirmed = false;
	};

	class ExpandMapDialog final : public Dialog
	{
	public:
		ExpandMapDialog(int& outAxis, int& outAmount);
		bool Draw() override;
	private:
		int _axis = 0; // 0=+Z, 1=-Z, 2=+X, 3=-X, 4=+Y, 5=-Y
		int _amount = 1;
		int& _outAxis;
		int& _outAmount;
		bool _confirmed = false;
	};

	class FileDialog final : public Dialog
	{
	public:
		FileDialog(std::string& outPath, const std::string& title,
			const std::string& extension, bool save);
		bool Draw() override;
		void SetCurrentPath(const std::filesystem::path& path);
	private:
		std::string _title;
		std::string _extension;
		bool _save;
		std::string _currentPath;
		char _fileName[260];
		std::string& _outPath;
		std::filesystem::path _baseDir;
		bool _confirmed = false;
	};

	class AssetPathDialog final : public Dialog
	{
	public:
		AssetPathDialog(std::string& outTexDir, std::string& outShapeDir);
		bool Draw() override;
	private:
		char _texDir[260];
		char _shapeDir[260];
		std::string& _outTexDir;
		std::string& _outShapeDir;
		bool _confirmed = false;
	};

	class SettingsDialog final : public Dialog
	{
	public:
		SettingsDialog(size_t& undoMax, float& mouseSens, bool& cullFaces, std::string& hideRegex);
		bool Draw() override;
	private:
		size_t& _undoMax;
		float& _mouseSens;
		bool& _cullFaces;
		std::string& _hideRegex;
		char _hideRegexBuf[260];
		bool _confirmed = false;
	};

	class AboutDialog final : public Dialog
	{
	public:
		AboutDialog();
		bool Draw() override;
	};

	class ShortcutsDialog final : public Dialog
	{
	public:
		ShortcutsDialog();
		bool Draw() override;
	};

	class InstructionsDialog final : public Dialog
	{
	public:
		InstructionsDialog();
		bool Draw() override;
	};

	class ExportDialog final : public Dialog
	{
	public:
		ExportDialog(std::string& outPath, bool& separateGeometry);
		bool Draw() override;
	private:
		char _path[260];
		bool _separate = false;
		std::string& _outPath;
		bool& _outSeparate;
		bool _confirmed = false;
	};

	class ConfirmationDialog final : public Dialog
	{
	public:
		ConfirmationDialog(const std::string& message, std::function<void(bool)> callback);
		bool Draw() override;
	private:
		std::string _message;
		std::function<void(bool)> _callback;
	};

} // namespace ed
