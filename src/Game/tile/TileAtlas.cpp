#include "stdafx.h"
#include "TileAtlas.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <cstdint>
#include <cstring>

namespace
{
	static uint8_t valueNoise(int x, int y, int seed)
	{
		uint32_t h = static_cast<uint32_t>(x * 374761393u + y * 668265263u + seed * 1274126177u);
		h = (h ^ (h >> 13)) * 1274126177u;
		h = h ^ (h >> 16);
		return static_cast<uint8_t>(h & 0xFF);
	}

	static void fillCheckerPattern(uint8_t* rgba, int w, int h, int stride, int cellSize,
		uint8_t r0, uint8_t g0, uint8_t b0,
		uint8_t r1, uint8_t g1, uint8_t b1)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * stride + x) * 4;
				bool c = ((x / cellSize) + (y / cellSize)) & 1;
				if (c) { rgba[idx] = r1; rgba[idx + 1] = g1; rgba[idx + 2] = b1; }
				else   { rgba[idx] = r0; rgba[idx + 1] = g0; rgba[idx + 2] = b0; }
				rgba[idx + 3] = 255;
			}
		}
	}

	static void fillStonePattern(uint8_t* rgba, int w, int h, int stride, int seed,
		uint8_t baseR, uint8_t baseG, uint8_t baseB)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * stride + x) * 4;
				uint8_t n = valueNoise(x, y, seed);
				int v = (static_cast<int>(n) - 128) / 4;
				rgba[idx]     = static_cast<uint8_t>(std::clamp(baseR + v, 0, 255));
				rgba[idx + 1] = static_cast<uint8_t>(std::clamp(baseG + v, 0, 255));
				rgba[idx + 2] = static_cast<uint8_t>(std::clamp(baseB + v, 0, 255));
				rgba[idx + 3] = 255;
			}
		}
		for (int y = 0; y < h; y += 16)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * stride + x) * 4;
				rgba[idx]     = static_cast<uint8_t>(rgba[idx] * 3 / 4);
				rgba[idx + 1] = static_cast<uint8_t>(rgba[idx + 1] * 3 / 4);
				rgba[idx + 2] = static_cast<uint8_t>(rgba[idx + 2] * 3 / 4);
			}
		}
		for (int x = 0; x < w; x += 16)
		{
			for (int y = 0; y < h; ++y)
			{
				int idx = (y * stride + x) * 4;
				rgba[idx]     = static_cast<uint8_t>(rgba[idx] * 3 / 4);
				rgba[idx + 1] = static_cast<uint8_t>(rgba[idx + 1] * 3 / 4);
				rgba[idx + 2] = static_cast<uint8_t>(rgba[idx + 2] * 3 / 4);
			}
		}
	}

	static void fillWoodPattern(uint8_t* rgba, int w, int h, int stride, int seed,
		uint8_t baseR, uint8_t baseG, uint8_t baseB)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * stride + x) * 4;
				uint8_t n = valueNoise(x * 2, y * 5, seed);
				int v = (static_cast<int>(n) - 128) / 3;
				rgba[idx]     = static_cast<uint8_t>(std::clamp(baseR + v, 0, 255));
				rgba[idx + 1] = static_cast<uint8_t>(std::clamp(baseG + v, 0, 255));
				rgba[idx + 2] = static_cast<uint8_t>(std::clamp(baseB + v, 0, 255));
				rgba[idx + 3] = 255;
			}
		}
		for (int y = 0; y < h; y += 3)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * stride + x) * 4;
				rgba[idx]     = static_cast<uint8_t>(rgba[idx] * 3 / 4);
				rgba[idx + 1] = static_cast<uint8_t>(rgba[idx + 1] * 3 / 4);
				rgba[idx + 2] = static_cast<uint8_t>(rgba[idx + 2] * 3 / 4);
			}
		}
	}

	static void fillMarblePattern(uint8_t* rgba, int w, int h, int stride, int seed,
		uint8_t baseR, uint8_t baseG, uint8_t baseB)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				int idx = (y * stride + x) * 4;
				uint8_t n = valueNoise(x, y, seed);
				uint8_t n2 = valueNoise(x + 50, y + 50, seed + 1);
				int v = (static_cast<int>(n) + static_cast<int>(n2)) / 2 - 128;
				v /= 3;
				rgba[idx]     = static_cast<uint8_t>(std::clamp(baseR + v, 0, 255));
				rgba[idx + 1] = static_cast<uint8_t>(std::clamp(baseG + v, 0, 255));
				rgba[idx + 2] = static_cast<uint8_t>(std::clamp(baseB + v, 0, 255));
				rgba[idx + 3] = 255;
			}
		}
		for (int i = 0; i < 5; ++i)
		{
			int lx = valueNoise(i, seed, 0) % w;
			int ly = valueNoise(i, seed, 1) % h;
			for (int j = 0; j < 20; ++j)
			{
				int px = lx + (valueNoise(j, i, seed) % 5) - 2;
				int py = ly + j * (h / 20);
				if (px >= 0 && px < w && py >= 0 && py < h)
				{
					int idx = (py * stride + px) * 4;
					rgba[idx]     = static_cast<uint8_t>(rgba[idx] * 2 / 3);
					rgba[idx + 1] = static_cast<uint8_t>(rgba[idx + 1] * 2 / 3);
					rgba[idx + 2] = static_cast<uint8_t>(rgba[idx + 2] * 2 / 3);
				}
			}
		}
	}
}

namespace tile
{
	static void fillOneCell(uint8_t* rgba, int tileSize, int stride, int localIdx, int seedShift)
	{
		switch (localIdx)
		{
		case 0: // wall (gray stone)
			fillStonePattern(rgba, tileSize, tileSize, stride, 42 + seedShift, 140, 130, 120);
			break;
		case 1: // floor (brown checker)
			fillCheckerPattern(rgba, tileSize, tileSize, stride, 8, 180, 140, 100, 160, 120, 80);
			break;
		case 2: // ceiling (dark gray)
			fillStonePattern(rgba, tileSize, tileSize, stride, 99 + seedShift, 60, 60, 65);
			break;
		case 3: // selected (yellow)
			fillCheckerPattern(rgba, tileSize, tileSize, stride, 4, 255, 255, 0, 200, 200, 0);
			break;
		case 4: // red brick
			fillStonePattern(rgba, tileSize, tileSize, stride, 17 + seedShift, 180, 60, 50);
			break;
		case 5: // blue
			fillStonePattern(rgba, tileSize, tileSize, stride, 33 + seedShift, 60, 100, 180);
			break;
		case 6: // green
			fillStonePattern(rgba, tileSize, tileSize, stride, 55 + seedShift, 80, 160, 70);
			break;
		case 7: // white marble
			fillMarblePattern(rgba, tileSize, tileSize, stride, 42 + seedShift, 200, 200, 195);
			break;
		case 8: // dark wood
			fillWoodPattern(rgba, tileSize, tileSize, stride, 77 + seedShift, 100, 60, 40);
			break;
		case 9: // cobblestone
			fillStonePattern(rgba, tileSize, tileSize, stride, 31 + seedShift, 110, 105, 100);
			break;
		case 10: // sandy stone
			fillStonePattern(rgba, tileSize, tileSize, stride, 58 + seedShift, 180, 170, 130);
			break;
		case 11: // purple crystal
			fillStonePattern(rgba, tileSize, tileSize, stride, 63 + seedShift, 120, 60, 160);
			break;
		case 12: // orange terracotta
			fillCheckerPattern(rgba, tileSize, tileSize, stride, 8, 200, 120, 70, 170, 100, 55);
			break;
		case 13: // teal tile
			fillCheckerPattern(rgba, tileSize, tileSize, stride, 16, 50, 160, 150, 35, 130, 120);
			break;
		case 14: // pink marble
			fillMarblePattern(rgba, tileSize, tileSize, stride, 88 + seedShift, 180, 120, 150);
			break;
		case 15: // obsidian
			fillStonePattern(rgba, tileSize, tileSize, stride, 101 + seedShift, 30, 35, 50);
			break;
		default: // procedural
		{
			uint32_t h = localIdx * 1640531527u;
			uint8_t r = static_cast<uint8_t>((h >> 16) & 0xFF);
			uint8_t g = static_cast<uint8_t>((h >> 8) & 0xFF);
			uint8_t b = static_cast<uint8_t>(h & 0xFF);
			int pattern = localIdx % 3;
			if (seedShift != 0)
			{
				// Shift colors for T2 so textures look distinct
				r = static_cast<uint8_t>((r + 80) & 0xFF);
				g = static_cast<uint8_t>((g + 40) & 0xFF);
				b = static_cast<uint8_t>((b + 120) & 0xFF);
			}
			if (pattern == 0)
				fillStonePattern(rgba, tileSize, tileSize, stride, localIdx * 37 + seedShift, r, g, b);
			else if (pattern == 1)
				fillCheckerPattern(rgba, tileSize, tileSize, stride, 8, r, g, b, r / 2, g / 2, b / 2);
			else
				fillMarblePattern(rgba, tileSize, tileSize, stride, localIdx * 73 + seedShift, r, g, b);
			break;
		}
		}
	}

	// Fill a range of texture cells in a pre-allocated atlas pixel buffer.
	// totalCols, totalRows — the full atlas grid dimensions.
	// firstGlobalIdx — global texture index of the first cell to fill.
	// count — number of cells to fill.
	// seedShift — added to all pattern seeds for variation.
	static void fillTextureRange(std::vector<uint8_t>& pixels, int tileSize,
		int totalCols, int totalRows, int firstGlobalIdx, int count, int seedShift)
	{
		(void)totalRows;
		int stride = tileSize * totalCols;
		for (int li = 0; li < count; ++li)
		{
			int gi = firstGlobalIdx + li;
			int ox = (gi % totalCols) * tileSize;
			int oy = (gi / totalCols) * tileSize;
			uint8_t* dst = &pixels[(oy * stride + ox) * 4];
			fillOneCell(dst, tileSize, stride, li, seedShift);
		}
	}

	static std::vector<uint8_t> generateGridPixels(int tileSize, int atlasDim)
	{
		int totalTex = atlasDim * atlasDim;
		uint32_t totalW = static_cast<uint32_t>(tileSize * atlasDim);
		uint32_t totalH = static_cast<uint32_t>(tileSize * atlasDim);
		std::vector<uint8_t> pixels(totalW * totalH * 4, 255);
		fillTextureRange(pixels, tileSize, atlasDim, atlasDim, 0, totalTex, 0);
		return pixels;
	}

	gpu::texture::TexturePtr CreateTileAtlas(int tileSize, int atlasDim)
	{
		auto pixels = generateGridPixels(tileSize, atlasDim);
		uint32_t totalW = static_cast<uint32_t>(tileSize * atlasDim);
		uint32_t totalH = static_cast<uint32_t>(tileSize * atlasDim);
		auto tex = gpu::texture::CreateTexture2D(
			{ totalW, totalH },
			gpu::Format::R8G8B8A8_UNORM,
			"tileAtlas");

		gpu::texture::TextureUpdateInfo update{};
		update.level  = 0;
		update.extent = { totalW, totalH, 1u };
		update.pixels = pixels.data();
		update.format = gpu::UploadFormat::RGBA;
		update.type   = gpu::UploadType::UBYTE;
		gpu::texture::UpdateImage(tex, update);

		return tex;
	}

	gpu::texture::TexturePtr CreateWallAtlas(int tileSize, int atlasDim)
	{
		// Classic square atlas: atlasDim × atlasDim grid, each cell tileSize × tileSize
		auto pixels = generateGridPixels(tileSize, atlasDim);
		uint32_t totalW = static_cast<uint32_t>(tileSize * atlasDim);
		uint32_t totalH = static_cast<uint32_t>(tileSize * atlasDim);

		auto tex = gpu::texture::CreateTexture2D(
			{ totalW, totalH },
			gpu::Format::R8G8B8A8_UNORM,
			"wallAtlas");

		gpu::texture::TextureUpdateInfo update{};
		update.level  = 0;
		update.extent = { totalW, totalH, 1u };
		update.pixels = pixels.data();
		update.format = gpu::UploadFormat::RGBA;
		update.type   = gpu::UploadType::UBYTE;
		gpu::texture::UpdateImage(tex, update);

		return tex;
	}

	gpu::texture::TexturePtr CreateCombinedWallAtlas(int tileSize)
	{
		int totalCols = 16;
		int totalRows = 8;
		uint32_t totalW = static_cast<uint32_t>(tileSize * totalCols);
		uint32_t totalH = static_cast<uint32_t>(tileSize * totalRows);
		std::vector<uint8_t> pixels(totalW * totalH * 4, 255);

		// T1 — cols 0-7 (global indices 0-63)
		fillTextureRange(pixels, tileSize, totalCols, totalRows, 0, 64, 0);
		// T2 — cols 8-15 (global indices 64-127)
		fillTextureRange(pixels, tileSize, totalCols, totalRows, 64, 64, 42);

		auto tex = gpu::texture::CreateTexture2D(
			{ totalW, totalH },
			gpu::Format::R8G8B8A8_UNORM,
			"combinedWallAtlas");

		gpu::texture::TextureUpdateInfo update{};
		update.level  = 0;
		update.extent = { totalW, totalH, 1u };
		update.pixels = pixels.data();
		update.format = gpu::UploadFormat::RGBA;
		update.type   = gpu::UploadType::UBYTE;
		gpu::texture::UpdateImage(tex, update);

		return tex;
	}
}
