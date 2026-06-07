#include "stdafx.h"
#include "tile.hpp"
#include "map_man/map_man.hpp"
//=============================================================================
namespace
{
	// Simple base64 encoder/decoder (no external dependency)
	static const char* B64_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string base64Encode(const std::vector<uint8_t>& bin)
	{
		std::string out;
		out.reserve(((bin.size() + 2) / 3) * 4);
		for (size_t i = 0; i < bin.size(); i += 3)
		{
			uint32_t b = (static_cast<uint32_t>(bin[i]) << 16)
				| (i + 1 < bin.size() ? static_cast<uint32_t>(bin[i + 1]) << 8 : 0)
				| (i + 2 < bin.size() ? static_cast<uint32_t>(bin[i + 2]) : 0);
			out.push_back(B64_CHARS[(b >> 18) & 0x3F]);
			out.push_back(B64_CHARS[(b >> 12) & 0x3F]);
			out.push_back(i + 1 < bin.size() ? B64_CHARS[(b >> 6) & 0x3F] : '=');
			out.push_back(i + 2 < bin.size() ? B64_CHARS[b & 0x3F] : '=');
		}
		return out;
	}

	std::vector<uint8_t> base64Decode(const std::string& str)
	{
		auto pos = [](char c) -> uint8_t
			{
				if (c >= 'A' && c <= 'Z') return c - 'A';
				if (c >= 'a' && c <= 'z') return c - 'a' + 26;
				if (c >= '0' && c <= '9') return c - '0' + 52;
				if (c == '+') return 62;
				if (c == '/') return 63;
				return 0;
			};
		std::vector<uint8_t> out;
		out.reserve((str.size() / 4) * 3);
		for (size_t i = 0; i < str.size(); i += 4)
		{
			uint32_t b = (static_cast<uint32_t>(pos(str[i])) << 18)
				| (static_cast<uint32_t>(pos(str[i + 1])) << 12)
				| (static_cast<uint32_t>(pos(str[i + 2])) << 6)
				| static_cast<uint32_t>(pos(str[i + 3]));
			out.push_back(static_cast<uint8_t>((b >> 16) & 0xFF));
			if (str[i + 2] != '=') out.push_back(static_cast<uint8_t>((b >> 8) & 0xFF));
			if (str[i + 3] != '=') out.push_back(static_cast<uint8_t>(b & 0xFF));
		}
		return out;
	}

	template<typename T>
	void AppendBytesLE(std::vector<uint8_t>& bytes, T value)
	{
		for (size_t b = 0; b < sizeof(T); ++b)
			bytes.push_back(static_cast<uint8_t>((value >> (b * 8)) & 0xFF));
	}
}
//=============================================================================
ed::TileGrid::TileGrid(MapMan& mapMan, size_t width, size_t height, size_t length)
	: TileGrid(mapMan, width, height, length, TILE_SPACING_DEFAULT, Tile())
{}
//=============================================================================
ed::TileGrid::TileGrid(MapMan& mapMan, size_t width, size_t height, size_t length, float spacing, Tile fill)
	: Grid<Tile>(width, height, length, spacing, fill)
	, _mapMan(mapMan)
{}
//=============================================================================
ed::Tile ed::TileGrid::GetTile(int i, int j, int k) const
{
	return GetCel(i, j, k);
}
//=============================================================================
ed::Tile ed::TileGrid::GetTile(int flatIndex) const
{
	return _grid[static_cast<size_t>(flatIndex)];
}
//=============================================================================
void ed::TileGrid::SetTile(int i, int j, int k, const Tile& tile)
{
	SetCel(i, j, k, tile);
}
//=============================================================================
void ed::TileGrid::SetTile(int flatIndex, const Tile& tile)
{
	_grid[static_cast<size_t>(flatIndex)] = tile;
}
//=============================================================================
void ed::TileGrid::SetTileRect(int i, int j, int k, int w, int h, int l, const Tile& tile)
{
	assert(i >= 0 && j >= 0 && k >= 0);
	assert(i + w <= static_cast<int>(_width) && j + h <= static_cast<int>(_height) && k + l <= static_cast<int>(_length));
	for (int y = j; y < j + h; ++y)
	{
		for (int z = k; z < k + l; ++z)
		{
			size_t base = FlatIndex(0, y, z);
			for (int x = i; x < i + w; ++x)
				_grid[base + static_cast<size_t>(x)] = tile;
		}
	}
}
//=============================================================================
void ed::TileGrid::CopyTiles(int i, int j, int k, const TileGrid& src, bool ignoreEmpty)
{
	assert(i >= 0 && j >= 0 && k >= 0);
	int xEnd = Min(i + static_cast<int>(src._width), static_cast<int>(_width));
	int yEnd = Min(j + static_cast<int>(src._height), static_cast<int>(_height));
	int zEnd = Min(k + static_cast<int>(src._length), static_cast<int>(_length));
	for (int z = k; z < zEnd; ++z)
	{
		for (int y = j; y < yEnd; ++y)
		{
			size_t ourBase = FlatIndex(0, y, z);
			size_t theirBase = src.FlatIndex(0, y - j, z - k);
			for (int x = i; x < xEnd; ++x)
			{
				const Tile& tile = src._grid[theirBase + static_cast<size_t>(x - i)];
				if (!ignoreEmpty || tile)
					_grid[ourBase + static_cast<size_t>(x)] = tile;
			}
		}
	}
}
//=============================================================================
void ed::TileGrid::UnsetTile(int i, int j, int k)
{
	_grid[FlatIndex(i, j, k)].shape = NO_MODEL;
}
//=============================================================================
ed::TileGrid ed::TileGrid::Subsection(int i, int j, int k, int w, int h, int l) const
{
	assert(i >= 0 && j >= 0 && k >= 0);
	assert(i + w <= static_cast<int>(_width) && j + h <= static_cast<int>(_height) && k + l <= static_cast<int>(_length));

	TileGrid newGrid(_mapMan, static_cast<size_t>(w), static_cast<size_t>(h), static_cast<size_t>(l));
	SubsectionCopy(i, j, k, w, h, l, newGrid);
	return newGrid;
}
//=============================================================================
std::string ed::TileGrid::GetTileDataBase64() const
{
	std::vector<uint8_t> bin;
	bin.reserve(_grid.size() * (sizeof(ModelID) + TEXTURES_PER_TILE * sizeof(TexID) + 2));

	ModelID runLength = 0;
	for (size_t i = 0; i < _grid.size(); ++i)
	{
		const Tile& savedTile = _grid[i];

		if (!savedTile && i < _grid.size() - 1 && runLength < INT16_MAX)
		{
			++runLength;
		}
		else
		{
			if (runLength > 0)
			{
				if (i == _grid.size() - 1) ++runLength;
				AppendBytesLE<ModelID>(bin, -runLength);
				if (savedTile) runLength = 0;
				else runLength = 1;
			}

			if (savedTile)
			{
				AppendBytesLE<ModelID>(bin, savedTile.shape);
				for (TexID id : savedTile.textures) AppendBytesLE<TexID>(bin, id);
				bin.push_back(savedTile.yaw);
				bin.push_back(savedTile.pitch);
			}
		}
	}

	return base64Encode(bin);
}
//=============================================================================
void ed::TileGrid::SetTileDataBase64OldFormat(std::string data)
{
	std::vector<uint8_t> bin = base64Decode(data);

	size_t gridIndex = 0, byteIndex = 0;
	while (byteIndex < bin.size())
	{
		int32_t oldModelID;
		memcpy(&oldModelID, &bin[byteIndex], sizeof(int32_t));
		ModelID modelID = static_cast<ModelID>(oldModelID);
		byteIndex += sizeof(int32_t);

		if (modelID < 0)
		{
			for (size_t t = 0; t < static_cast<size_t>(-modelID); ++t)
				_grid[gridIndex + t] = Tile();
			gridIndex += static_cast<size_t>(-modelID);
			byteIndex += 3 * sizeof(int32_t);
			continue;
		}

		int32_t angle;
		memcpy(&angle, &bin[byteIndex], sizeof(int32_t));
		uint8_t yaw = static_cast<uint8_t>((angle % 360) / 90);
		byteIndex += sizeof(int32_t);

		int32_t oldTexID;
		memcpy(&oldTexID, &bin[byteIndex], sizeof(int32_t));
		TexID texID = static_cast<TexID>(oldTexID);
		byteIndex += sizeof(int32_t);

		int32_t oldPitch;
		memcpy(&oldPitch, &bin[byteIndex], sizeof(int32_t));
		uint8_t pitch = static_cast<uint8_t>((oldPitch % 360) / 90);
		byteIndex += sizeof(int32_t);

		_grid[gridIndex] = Tile(modelID, texID, texID, yaw, pitch);
		++gridIndex;
	}
}
//=============================================================================
void ed::TileGrid::SetTileDataBase64(std::string data)
{
	std::vector<uint8_t> bin = base64Decode(data);

	size_t gridIndex = 0, byteIndex = 0;
	while (byteIndex < bin.size())
	{
		ModelID modelID;
		memcpy(&modelID, &bin[byteIndex], sizeof(ModelID));
		byteIndex += sizeof(ModelID);

		if (modelID < 0)
		{
			for (size_t t = 0; t < static_cast<size_t>(-modelID); ++t)
				_grid[gridIndex + t] = Tile();
			gridIndex += static_cast<size_t>(-modelID);
			continue;
		}

		TexID tex1ID;
		memcpy(&tex1ID, &bin[byteIndex], sizeof(TexID));
		byteIndex += sizeof(TexID);

		TexID tex2ID;
		memcpy(&tex2ID, &bin[byteIndex], sizeof(TexID));
		byteIndex += sizeof(TexID);

		uint8_t yaw = bin[byteIndex++];
		uint8_t pitch = bin[byteIndex++];

		_grid[gridIndex] = Tile(modelID, tex1ID, tex2ID, yaw, pitch);
		++gridIndex;
	}
}
//=============================================================================
std::pair<std::vector<ed::TexID>, std::vector<ed::ModelID>> ed::TileGrid::GetUsedIDs() const
{
	std::set<TexID> usedTexIDs;
	std::set<ModelID> usedModelIDs;
	for (const Tile& tile : _grid)
	{
		if (tile)
		{
			for (TexID tex : tile.textures) usedTexIDs.insert(tex);
			usedModelIDs.insert(tile.shape);
		}
	}
	return std::make_pair(
		std::vector(usedTexIDs.begin(), usedTexIDs.end()),
		std::vector(usedModelIDs.begin(), usedModelIDs.end()));
}
//=============================================================================