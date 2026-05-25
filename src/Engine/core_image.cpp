#include "stdafx.h"
#include "core_image.h"
#include "core_log.h"
//=============================================================================
inline core::ImageFormat GetImageFormatFromChannels(int channels)
{
	switch (channels)
	{
	case 1: return core::ImageFormat::Red;
	case 2: return core::ImageFormat::RG;
	case 3: return core::ImageFormat::RGB;
	case 4: return core::ImageFormat::RGBA;
	default:
		core::Error("Unsupported number of channels in image: " + std::to_string(channels));
		return core::ImageFormat::RGB; // default to RGB
	}
}
//=============================================================================
void core::ImageData::Free()
{
	if (data)
	{
		stbi_image_free(data);
		data = nullptr;
	}
}
//=============================================================================
core::ImageData core::LoadImageFromFile(const std::string& filePath, bool flipVertically)
{
	ImageData image{};

	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(flipVertically);
	unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		image.width = width;
		image.height = height;
		image.channels = GetImageFormatFromChannels(nrChannels);
		image.data = data;
	}
	else
	{
		Error("Failed to load image: " + filePath);
	}

	return image;
}
//=============================================================================