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
	static std::vector<uint8_t> generateGridPixels(int tileSize, int atlasDim)
	{
		uint32_t totalW = static_cast<uint32_t>(tileSize * atlasDim);
		uint32_t totalH = static_cast<uint32_t>(tileSize * atlasDim);
		std::vector<uint8_t> pixels(totalW * totalH * 4, 255);

		auto texPos = [&](int idx) -> std::pair<int,int> {
			return { (idx % atlasDim) * tileSize, (idx / atlasDim) * tileSize };
		};
		// tex 0 = wall (gray stone)
		{
			auto [ox, oy] = texPos(0);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 42, 140, 130, 120);
		}
		// tex 1 = floor (brown checker)
		{
			auto [ox, oy] = texPos(1);
			fillCheckerPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 8, 180, 140, 100, 160, 120, 80);
		}
		// tex 2 = ceiling (dark gray)
		{
			auto [ox, oy] = texPos(2);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 99, 60, 60, 65);
		}
		// tex 3 = selected (yellow)
		{
			auto [ox, oy] = texPos(3);
			fillCheckerPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 4, 255, 255, 0, 200, 200, 0);
		}
		// tex 4 = red brick
		{
			auto [ox, oy] = texPos(4);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 17, 180, 60, 50);
		}
		// tex 5 = blue
		{
			auto [ox, oy] = texPos(5);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 33, 60, 100, 180);
		}
		// tex 6 = green
		{
			auto [ox, oy] = texPos(6);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 55, 80, 160, 70);
		}
		// tex 7 = white marble
		{
			auto [ox, oy] = texPos(7);
			fillMarblePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 42, 200, 200, 195);
		}
		// tex 8 = dark wood
		{
			auto [ox, oy] = texPos(8);
			fillWoodPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 77, 100, 60, 40);
		}
		// tex 9 = cobblestone
		{
			auto [ox, oy] = texPos(9);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 31, 110, 105, 100);
		}
		// tex 10 = sandy stone
		{
			auto [ox, oy] = texPos(10);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 58, 180, 170, 130);
		}
		// tex 11 = purple crystal
		{
			auto [ox, oy] = texPos(11);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 63, 120, 60, 160);
		}
		// tex 12 = orange terracotta
		{
			auto [ox, oy] = texPos(12);
			fillCheckerPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 8, 200, 120, 70, 170, 100, 55);
		}
		// tex 13 = teal tile
		{
			auto [ox, oy] = texPos(13);
			fillCheckerPattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 16, 50, 160, 150, 35, 130, 120);
		}
		// tex 14 = pink marble
		{
			auto [ox, oy] = texPos(14);
			fillMarblePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 88, 180, 120, 150);
		}
		// tex 15 = obsidian
		{
			auto [ox, oy] = texPos(15);
			fillStonePattern(&pixels[(oy * totalW + ox) * 4], tileSize, tileSize, totalW, 101, 30, 35, 50);
		}
		// tex 16+ fill remaining with varied patterns
		for (int idx = 16; idx < atlasDim * atlasDim; ++idx)
		{
			int ox = (idx % atlasDim) * tileSize;
			int oy = (idx / atlasDim) * tileSize;
			uint32_t h = idx * 1640531527u;
			uint8_t r = static_cast<uint8_t>((h >> 16) & 0xFF);
			uint8_t g = static_cast<uint8_t>((h >> 8) & 0xFF);
			uint8_t b = static_cast<uint8_t>(h & 0xFF);
			int pattern = idx % 3;
			uint8_t* dst = &pixels[(oy * totalW + ox) * 4];
			if (pattern == 0)
				fillStonePattern(dst, tileSize, tileSize, totalW, idx * 37, r, g, b);
			else if (pattern == 1)
				fillCheckerPattern(dst, tileSize, tileSize, totalW, 8, r, g, b, r/2, g/2, b/2);
			else
				fillMarblePattern(dst, tileSize, tileSize, totalW, idx * 73, r, g, b);
		}

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

	gpu::texture::TexturePtr CreateRepeatingWallAtlas(int tileSize, int atlasDim)
	{
		// Generate standard grid pixels first
		auto gridPixels = generateGridPixels(tileSize, atlasDim);
		uint32_t gridW = static_cast<uint32_t>(tileSize * atlasDim);

		// Repeating atlas: atlasDim² columns, each tileSize wide, atlasDim * tileSize tall
		uint32_t repW = static_cast<uint32_t>(tileSize * atlasDim * atlasDim);
		uint32_t repH = static_cast<uint32_t>(tileSize * atlasDim);
		std::vector<uint8_t> repPixels(repW * repH * 4, 255);
		int bpp = 4;

		for (int ti = 0; ti < atlasDim * atlasDim; ++ti)
		{
			int srcX = (ti % atlasDim) * tileSize;
			int srcY = (ti / atlasDim) * tileSize;

			// Destination column for this tile index
			int dstCol = ti * tileSize;

			// Copy tile atlasDim times stacked vertically
			for (int rep = 0; rep < atlasDim; ++rep)
			{
				int dstY = rep * tileSize;
				for (int y = 0; y < tileSize; ++y)
				{
					const uint8_t* srcRow = &gridPixels[((srcY + y) * gridW + srcX) * bpp];
					uint8_t* dstRow = &repPixels[((dstY + y) * repW + dstCol) * bpp];
					memcpy(dstRow, srcRow, static_cast<size_t>(tileSize) * bpp);
				}
			}
		}

		auto tex = gpu::texture::CreateTexture2D(
			{ repW, repH },
			gpu::Format::R8G8B8A8_UNORM,
			"wallAtlas");

		gpu::texture::TextureUpdateInfo update{};
		update.level  = 0;
		update.extent = { repW, repH, 1u };
		update.pixels = repPixels.data();
		update.format = gpu::UploadFormat::RGBA;
		update.type   = gpu::UploadType::UBYTE;
		gpu::texture::UpdateImage(tex, update);

		return tex;
	}
}
