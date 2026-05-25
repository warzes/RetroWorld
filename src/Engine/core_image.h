#pragma once

namespace core
{
	enum class ImageFormat : uint8_t
	{
		Red,
		RG,
		RGB,
		RGBA
	};

	struct ImageData final
	{
		bool IsValid() const noexcept { return width > 0 && height > 0 && data != nullptr; }
		void Free();

		int            width{ 0 };
		int            height{ 0 };
		ImageFormat    channels{};
		unsigned char* data{ nullptr };
	};

	ImageData LoadImageFromFile(const std::string& filePath, bool flipVertically = true);

} // namespace core