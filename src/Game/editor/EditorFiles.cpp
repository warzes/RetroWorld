#include "stdafx.h"
#include "Editor.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace
{
	//=====================================================================
	const std::string MAPS_DIR = "data/maps/";

	//=====================================================================
	[[nodiscard]] std::string sanitizeName(std::string_view name) noexcept
	{
		std::string result;
		result.reserve(name.size());
		for (char c : name)
		{
			if (isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-')
				result += c;
			else
				result += '_';
		}
		return result;
	}

	//=====================================================================
	void ensureMapsDir() noexcept
	{
		std::error_code ec;
		std::filesystem::create_directories(MAPS_DIR, ec);
	}

	//=====================================================================
	json tileToJson(const tile::Tile& t) noexcept
	{
		json j;
		j["spaceType"]        = static_cast<int>(t.spaceType);
		j["renderSolid"]      = t.renderSolid;
		j["wallTex"]          = t.wallTex;
		j["wallBottomTex"]    = t.wallBottomTex;
		j["floorTex"]         = t.floorTex;
		j["ceilTex"]          = t.ceilTex;
		j["wallAtlas"]        = t.wallAtlas;
		j["wallBottomAtlas"]  = t.wallBottomAtlas;
		j["floorAtlas"]       = t.floorAtlas;
		j["ceilAtlas"]        = t.ceilAtlas;
		j["northTex"]         = t.northTex;
		j["southTex"]         = t.southTex;
		j["eastTex"]          = t.eastTex;
		j["westTex"]          = t.westTex;
		j["northAtlas"]       = t.northAtlas;
		j["southAtlas"]       = t.southAtlas;
		j["eastAtlas"]        = t.eastAtlas;
		j["westAtlas"]        = t.westAtlas;
		j["bottomNorthTex"]   = t.bottomNorthTex;
		j["bottomSouthTex"]   = t.bottomSouthTex;
		j["bottomEastTex"]    = t.bottomEastTex;
		j["bottomWestTex"]    = t.bottomWestTex;
		j["bottomNorthAtlas"] = t.bottomNorthAtlas;
		j["bottomSouthAtlas"] = t.bottomSouthAtlas;
		j["bottomEastAtlas"]  = t.bottomEastAtlas;
		j["bottomWestAtlas"]  = t.bottomWestAtlas;
		j["floorHeight"]      = t.floorHeight;
		j["ceilHeight"]       = t.ceilHeight;
		j["slopeNW"]          = t.slopeNW;
		j["slopeNE"]          = t.slopeNE;
		j["slopeSE"]          = t.slopeSE;
		j["slopeSW"]          = t.slopeSW;
		j["ceilSlopeNW"]      = t.ceilSlopeNW;
		j["ceilSlopeNE"]      = t.ceilSlopeNE;
		j["ceilSlopeSE"]      = t.ceilSlopeSE;
		j["ceilSlopeSW"]      = t.ceilSlopeSW;
		return j;
	}

	//=====================================================================
	void jsonToTile(const json& j, tile::Tile& t) noexcept
	{
		t.spaceType        = static_cast<tile::TileSpaceType>(j.value("spaceType", 0));
		t.renderSolid      = j.value("renderSolid", false);
		t.wallTex          = static_cast<uint8_t>(j.value("wallTex", 0));
		t.wallBottomTex    = static_cast<uint8_t>(j.value("wallBottomTex", 0));
		t.floorTex         = static_cast<uint8_t>(j.value("floorTex", 1));
		t.ceilTex          = static_cast<uint8_t>(j.value("ceilTex", 2));
		t.wallAtlas        = static_cast<uint8_t>(j.value("wallAtlas", 0));
		t.wallBottomAtlas  = static_cast<uint8_t>(j.value("wallBottomAtlas", 0));
		t.floorAtlas       = static_cast<uint8_t>(j.value("floorAtlas", 0));
		t.ceilAtlas        = static_cast<uint8_t>(j.value("ceilAtlas", 0));
		t.northTex         = static_cast<uint8_t>(j.value("northTex", tile::TEX_NOT_SET));
		t.southTex         = static_cast<uint8_t>(j.value("southTex", tile::TEX_NOT_SET));
		t.eastTex          = static_cast<uint8_t>(j.value("eastTex", tile::TEX_NOT_SET));
		t.westTex          = static_cast<uint8_t>(j.value("westTex", tile::TEX_NOT_SET));
		t.northAtlas       = static_cast<uint8_t>(j.value("northAtlas", 0));
		t.southAtlas       = static_cast<uint8_t>(j.value("southAtlas", 0));
		t.eastAtlas        = static_cast<uint8_t>(j.value("eastAtlas", 0));
		t.westAtlas        = static_cast<uint8_t>(j.value("westAtlas", 0));
		t.bottomNorthTex   = static_cast<uint8_t>(j.value("bottomNorthTex", tile::TEX_NOT_SET));
		t.bottomSouthTex   = static_cast<uint8_t>(j.value("bottomSouthTex", tile::TEX_NOT_SET));
		t.bottomEastTex    = static_cast<uint8_t>(j.value("bottomEastTex", tile::TEX_NOT_SET));
		t.bottomWestTex    = static_cast<uint8_t>(j.value("bottomWestTex", tile::TEX_NOT_SET));
		t.bottomNorthAtlas = static_cast<uint8_t>(j.value("bottomNorthAtlas", 0));
		t.bottomSouthAtlas = static_cast<uint8_t>(j.value("bottomSouthAtlas", 0));
		t.bottomEastAtlas  = static_cast<uint8_t>(j.value("bottomEastAtlas", 0));
		t.bottomWestAtlas  = static_cast<uint8_t>(j.value("bottomWestAtlas", 0));
		t.floorHeight      = j.value("floorHeight", -0.5f);
		t.ceilHeight       = j.value("ceilHeight", 0.5f);
		t.slopeNW          = j.value("slopeNW", 0.0f);
		t.slopeNE          = j.value("slopeNE", 0.0f);
		t.slopeSE          = j.value("slopeSE", 0.0f);
		t.slopeSW          = j.value("slopeSW", 0.0f);
		t.ceilSlopeNW      = j.value("ceilSlopeNW", 0.0f);
		t.ceilSlopeNE      = j.value("ceilSlopeNE", 0.0f);
		t.ceilSlopeSE      = j.value("ceilSlopeSE", 0.0f);
		t.ceilSlopeSW      = j.value("ceilSlopeSW", 0.0f);
	}

	//=====================================================================
	[[nodiscard]] std::vector<std::string> listMapFiles() noexcept
	{
		ensureMapsDir();
		std::vector<std::string> files;
		std::error_code ec;
		for (const auto& entry : std::filesystem::directory_iterator(MAPS_DIR, ec))
		{
			if (entry.is_regular_file() && entry.path().extension() == ".json")
				files.push_back(entry.path().stem().string());
		}
		std::sort(files.begin(), files.end());
		return files;
	}
}

//=====================================================================
void NewMap() noexcept
{
	g_tileMap.Clear();
	g_selTX = g_selTY = -1;
	g_anchorTX = g_anchorTY = -1;
	g_selW = 1; g_selH = 1;
	g_selFace = tile::FaceDir::COUNT;
	g_selCorner = -1;
	g_dirtyMesh = true;
	g_currentMapPath.clear();
	g_mapName = "untitled";
}

//=====================================================================
bool SaveMap(std::string_view path) noexcept
{
	ensureMapsDir();

	json root;
	root["version"] = 1;
	root["name"]    = std::string(g_mapName);
	root["width"]   = g_tileMap.GetWidth();
	root["height"]  = g_tileMap.GetHeight();

	json tilesArr = json::array();
	int w = g_tileMap.GetWidth();
	int h = g_tileMap.GetHeight();

	for (int ty = 0; ty < h; ++ty)
		for (int tx = 0; tx < w; ++tx)
			tilesArr.push_back(tileToJson(g_tileMap.Get(tx, ty)));

	root["tiles"] = std::move(tilesArr);

	std::string pathStr(path);
	std::ofstream ofs(pathStr, std::ios::ate | std::ios::binary);
	if (!ofs.is_open())
		return false;

	ofs << root.dump(1, '\t') << "\n";
	return true;
}

//=====================================================================
bool SaveMapToPath(std::string_view name) noexcept
{
	std::string safe = sanitizeName(name);
	if (safe.empty()) return false;
	g_mapName   = safe;
	g_currentMapPath = MAPS_DIR + safe + ".json";
	return SaveMap(g_currentMapPath);
}

//=====================================================================
bool LoadMap(std::string_view path) noexcept
{
	std::string pathStr(path);
	std::ifstream ifs{ pathStr };
	if (!ifs.is_open())
		return false;

	json root;
	try
	{
		ifs >> root;
	}
	catch (...)
	{
		return false;
	}

	int version = root.value("version", 0);
	if (version < 1) return false;

	int w = root.value("width", 16);
	int h = root.value("height", 16);

	g_tileMap.Resize(w, h);

	auto& tilesArr = root["tiles"];
	if (tilesArr.is_array())
	{
		size_t wh = static_cast<size_t>(w) * static_cast<size_t>(h);
		size_t count = std::min(tilesArr.size(), wh);

		for (size_t i = 0; i < count; ++i)
		{
			int tx = static_cast<int>(i % static_cast<size_t>(w));
			int ty = static_cast<int>(i / static_cast<size_t>(w));
			jsonToTile(tilesArr[i], g_tileMap.Get(tx, ty));
		}
	}

	g_mapName = root.value("name", "untitled");
	g_currentMapPath = std::string(path);

	g_selTX = g_selTY = -1;
	g_anchorTX = g_anchorTY = -1;
	g_selW = 1; g_selH = 1;
	g_selFace = tile::FaceDir::COUNT;
	g_selCorner = -1;
	g_dirtyMesh = true;

	return true;
}

//=====================================================================
std::vector<std::string> ListSavedMaps() noexcept
{
	return listMapFiles();
}
