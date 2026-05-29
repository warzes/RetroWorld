#pragma once

#include "gpu_core.h"
#include "core_baseTypes.h"

namespace gpu::texture
{
	struct Texture;
	using TexturePtr = std::shared_ptr<Texture>;

	struct TextureView;
	using TextureViewPtr = std::shared_ptr<TextureView>;

	struct Sampler;
	using SamplerPtr = std::shared_ptr<Sampler>;

	struct CopyBufferToTextureInfo;

	struct TextureCreateInfo final
	{
		bool operator==(const TextureCreateInfo&) const noexcept = default;

		ImageType      imageType = {};
		Format         format = {};
		core::Extent3D extent = {};
		uint32_t       mipLevels = 0;
		uint32_t       arrayLayers = 0;
		SampleCount    sampleCount = {};
	};

	struct TextureUpdateInfo final
	{
		uint32_t       level = 0;
		core::Offset3D offset = {};
		core::Extent3D extent = {};
		UploadFormat   format = UploadFormat::INFER_FORMAT;
		UploadType     type = UploadType::INFER_TYPE;
		const void*    pixels = nullptr;

		// Specifies, in texels, the size of rows in the array (for 2D and 3D images). If zero, it is assumed to be tightly packed according to size
		uint32_t       rowLength = 0;

		// Specifies, in texels, the number of rows in the array (for 3D images. If zero, it is assumed to be tightly packed according to size
		uint32_t       imageHeight = 0;
	};

	struct CompressedTextureUpdateInfo final
	{
		uint32_t       level = 0;
		core::Offset3D offset = {};
		core::Extent3D extent = {};
		const void*    data = nullptr;
	};

	struct TextureClearInfo final
	{
		uint32_t       level = 0;
		core::Offset3D offset = {};
		core::Extent3D extent = {};
		UploadFormat   format = UploadFormat::INFER_FORMAT;
		UploadType     type = UploadType::INFER_TYPE;

		// If null, then the subresource will be cleared with zeroes
		const void*    data = nullptr;
	};

	struct ComponentMapping final
	{
		ComponentSwizzle r = ComponentSwizzle::R;
		ComponentSwizzle g = ComponentSwizzle::G;
		ComponentSwizzle b = ComponentSwizzle::B;
		ComponentSwizzle a = ComponentSwizzle::A;
	};

	struct TextureViewCreateInfo final
	{
		ImageType        viewType = {};
		Format           format = {};
		ComponentMapping components = {};
		uint32_t         minLevel = 0;
		uint32_t         numLevels = 0;
		uint32_t         minLayer = 0;
		uint32_t         numLayers = 0;
	};

	struct SamplerState final
	{
		bool operator==(const SamplerState& rhs) const noexcept = default;

		float       lodBias{ 0.0f };
		float       minLod{ -1000.0f };
		float       maxLod{ 1000.0f };

		Filter      minFilter = Filter::Linear;
		Filter      magFilter = Filter::Linear;
		Filter      mipmapFilter = Filter::None;
		AddressMode addressModeU = AddressMode::ClampToEdge;
		AddressMode addressModeV = AddressMode::ClampToEdge;
		AddressMode addressModeW = AddressMode::ClampToEdge;
		BorderColor borderColor = BorderColor::FloatOpaqueWhite;
		SampleCount anisotropy = SampleCount::Samples1;
		bool        compareEnable = false;
		CompareOp   compareOp = CompareOp::Never;
	};

	[[nodiscard]] TexturePtr CreateTexture(const TextureCreateInfo& createInfo, std::string_view name = "");
	[[nodiscard]] TexturePtr CreateTexture2D(core::Extent2D size, Format format, std::string_view name = "");
	[[nodiscard]] TexturePtr CreateTexture2DMip(core::Extent2D size, Format format, uint32_t mipLevels, std::string_view name = "");

	[[nodiscard]] TexturePtr LoadTexture2D(std::string_view path);

	[[nodiscard]] TextureViewPtr CreateTextureView(const TextureViewCreateInfo& viewInfo, TexturePtr texture, std::string_view name = "");
	[[nodiscard]] TextureViewPtr CreateTextureView(const TextureViewCreateInfo& viewInfo, TextureViewPtr textureView, std::string_view name = "");
	[[nodiscard]] TextureViewPtr CreateTextureView(TexturePtr texture, std::string_view name = "");

	// Creates a view of a single mip level of the image
	[[nodiscard]] TextureViewPtr CreateSingleMipView(TexturePtr texture, uint32_t level);
	// Creates a view of a single array layer of the image
	[[nodiscard]] TextureViewPtr CreateSingleLayerView(TexturePtr texture, uint32_t layer);
	// Reinterpret the data of this texture
	[[nodiscard]] TextureViewPtr CreateFormatView(TexturePtr texture, Format newFormat);
	// Creates a view of the texture with a new component mapping
	[[nodiscard]] TextureViewPtr CreateSwizzleView(TexturePtr texture, ComponentMapping components);

	[[nodiscard]] SamplerPtr CreateSampler(const SamplerState& samplerState);

	// Generates and makes resident a bindless handle from the image and a sampler.
	[[nodiscard]] uint64_t GetBindlessHandle(const TexturePtr& texture, const SamplerPtr& sampler);

	// Automatically generates LoDs of the image. All mip levels beyond 0 are filled with the generated LoDs
	void GenMipmaps(const TexturePtr& texture);

	[[nodiscard]] const TextureCreateInfo& GetCreateInfo(const TexturePtr& texture) noexcept;
	[[nodiscard]] core::Extent3D Extent(const TexturePtr& texture) noexcept;
	[[nodiscard]] uint32_t Handle(const TexturePtr& texture) noexcept;
	[[nodiscard]] bool IsValid(const TexturePtr& texture) noexcept;

	[[nodiscard]] const TextureCreateInfo& GetCreateInfo(const TextureViewPtr& view) noexcept;
	[[nodiscard]] const TextureViewCreateInfo& GetViewInfo(const TextureViewPtr& view) noexcept;
	[[nodiscard]] core::Extent3D Extent(const TextureViewPtr& view) noexcept;
	[[nodiscard]] uint32_t Handle(const TextureViewPtr& view) noexcept;
	[[nodiscard]] bool IsValid(const TextureViewPtr& view) noexcept;

	[[nodiscard]] uint32_t Handle(const SamplerPtr& sampler) noexcept;
	[[nodiscard]] bool IsValid(const SamplerPtr& sampler) noexcept;

	void UpdateImage(const TexturePtr& texture, const TextureUpdateInfo& info);
	void UpdateCompressedImage(const TexturePtr& texture, const CompressedTextureUpdateInfo& info);
	void ClearImage(const TexturePtr& texture, const TextureClearInfo& info);

} // namespace gpu::texture

template<>
struct std::hash<gpu::texture::SamplerState>
{
	std::size_t operator()(const gpu::texture::SamplerState& k) const noexcept;
};