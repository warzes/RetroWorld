#include "stdafx.h"
#include "res_textureLoader.h"
//=============================================================================
std::unordered_map<std::string, gpu::texture::TexturePtr> texturesCache;
//=============================================================================
gpu::texture::TexturePtr res::GetOrLoadTexture2D(const std::string filename)
{
	if (auto it = texturesCache.find(filename); it != texturesCache.end())
		return it->second;

	int x = 0;
	int y = 0;
	const auto imageData = stbi_load(filename.c_str(), &x, &y, nullptr, 4);
	assert(imageData); // TODO:
	auto texture = gpu::texture::CreateTexture2D({ static_cast<uint32_t>(x), static_cast<uint32_t>(y) }, gpu::Format::R8G8B8A8_UNORM);
	UpdateImage(texture, {
	  .extent = {static_cast<uint32_t>(x), static_cast<uint32_t>(y)},
	  .format = gpu::UploadFormat::RGBA,
	  .type = gpu::UploadType::UBYTE,
	  .pixels = imageData,
		});
	stbi_image_free(imageData);

	return texturesCache.insert({ filename, texture }).first->second;
}
//=============================================================================