#pragma once

#include <gpu_texture.h>

namespace tile
{
	gpu::texture::TexturePtr CreateTileAtlas(int tileSize = 64, int atlasDim = 4);
	gpu::texture::TexturePtr CreateWallAtlas(int tileSize = 64, int atlasDim = 8);
}
