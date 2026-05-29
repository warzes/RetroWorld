#pragma once

#include "gpu_texture.h"

namespace res
{
	gpu::texture::TexturePtr GetOrLoadTexture2D(const std::string filename);
} // namespace res