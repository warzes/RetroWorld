#pragma once

#include <gpu_texture.h>

namespace tile
{
	gpu::texture::TexturePtr CreateTileAtlas(int tileSize = 64, int atlasDim = 4);
	gpu::texture::TexturePtr CreateWallAtlas(int tileSize = 64, int atlasDim = 8);

	// Combined 16×8 mega-atlas: cols 0-7 = T1 (64 textures), cols 8-15 = T2 (64 textures)
	gpu::texture::TexturePtr CreateCombinedWallAtlas(int tileSize = 64);
}
