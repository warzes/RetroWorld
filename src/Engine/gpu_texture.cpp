#include "stdafx.h"
#include "gpu_texture.h"
#include "_gpu_enumDesc.h"
#include "core_log.h"
#include "_gpu_contextState.h"
#include "core_utils.h"
//=============================================================================
std::size_t std::hash<gpu::texture::SamplerState>::operator()(const gpu::texture::SamplerState& k) const noexcept
{
	auto rtup = std::make_tuple(k.minFilter,
		k.magFilter,
		k.mipmapFilter,
		k.addressModeU,
		k.addressModeV,
		k.addressModeW,
		k.borderColor,
		k.anisotropy,
		k.compareEnable,
		k.compareOp,
		k.lodBias,
		k.minLod,
		k.maxLod);
	return core::Hash<decltype(rtup)>{}(rtup);
}
//=============================================================================
inline uint64_t getBlockCompressedImageSize(gpu::Format format, uint32_t width, uint32_t height, uint32_t depth) noexcept
{
	assert(gpu::IsBlockCompressedFormat(format));

	// BCn formats store 4x4 blocks of pixels, even if the dimensions aren't a multiple of 4
	// We round up to the nearest multiple of 4 for width and height, but not depth, since
	// 3D BCn images are just multiple 2D images stacked
	width = (width + 4 - 1) & -4;
	height = (height + 4 - 1) & -4;

	switch (format)
	{
		// BC1 and BC4 store 4x4 blocks with 64 bits (8 bytes)
	case gpu::Format::BC1_RGB_UNORM:
	case gpu::Format::BC1_RGBA_UNORM:
	case gpu::Format::BC1_RGB_SRGB:
	case gpu::Format::BC1_RGBA_SRGB:
	case gpu::Format::BC4_R_UNORM:
	case gpu::Format::BC4_R_SNORM:
		return width * height * depth / 2;

	// BC3, BC5, BC6, and BC7 store 4x4 blocks with 128 bits (16 bytes)
	case gpu::Format::BC2_RGBA_UNORM:
	case gpu::Format::BC2_RGBA_SRGB:
	case gpu::Format::BC3_RGBA_UNORM:
	case gpu::Format::BC3_RGBA_SRGB:
	case gpu::Format::BC5_RG_UNORM:
	case gpu::Format::BC5_RG_SNORM:
	case gpu::Format::BC6H_RGB_UFLOAT:
	case gpu::Format::BC6H_RGB_SFLOAT:
	case gpu::Format::BC7_RGBA_UNORM:
	case gpu::Format::BC7_RGBA_SRGB:
		return width * height * depth;
	default: std::unreachable();
	}
}
//=============================================================================
struct gpu::texture::Texture final
{
	Texture() noexcept = default;
	~Texture()
	{
		if (id)
		{
			if (bindlessHandle != 0)
				glMakeTextureHandleNonResidentARB(bindlessHandle);

			core::Debug("Destroyed texture with handle " + std::to_string(id));
			glDeleteTextures(1, &id);
		}
	}

	Texture(const Texture&) = delete;
	Texture& operator=(const Texture&) = delete;
	Texture(Texture&&) noexcept = default;
	Texture& operator=(Texture&&) noexcept = default;

	[[nodiscard]] core::Extent3D Extent() const noexcept { return createInfo.extent; }
	[[nodiscard]] operator bool() const noexcept { return id > 0; }
	[[nodiscard]] unsigned Handle() const noexcept { return id; }
	[[nodiscard]] bool IsValid() const noexcept { return id > 0; }

	uint32_t          id{ 0 };
	TextureCreateInfo createInfo{};
	uint64_t          bindlessHandle = 0;
};
//=============================================================================
struct gpu::texture::TextureView final
{
	TextureView() noexcept = default;
	~TextureView() = default; // TODO:???

	TextureView(const TextureView&) = delete;
	TextureView& operator=(const TextureView&) = delete;
	TextureView(TextureView&&) noexcept = default;
	TextureView& operator=(TextureView&&) noexcept = default;

	[[nodiscard]] core::Extent3D Extent() const noexcept { return createInfo.extent; }
	[[nodiscard]] operator bool() const noexcept { return id > 0; }
	[[nodiscard]] unsigned Handle() const noexcept { return id; }
	[[nodiscard]] bool IsValid() const noexcept { return id > 0; }

	uint32_t              id{ 0 };
	TextureCreateInfo     createInfo{};
	TextureViewCreateInfo viewInfo{};
};
//=============================================================================
struct gpu::texture::Sampler final
{
	Sampler() noexcept
	{
		glCreateSamplers(1, &id);
	}
	~Sampler()
	{
		if (id)
		{
			core::Debug("Destroyed sampler with handle " + std::to_string(id));
			glDeleteSamplers(1, &id);
		}
	}

	Sampler(const Sampler&) = delete;
	Sampler& operator=(const Sampler&) = delete;
	Sampler(Sampler&&) noexcept = default;
	Sampler& operator=(Sampler&&) noexcept = default;

	[[nodiscard]] operator bool() const noexcept { return id > 0; }
	[[nodiscard]] unsigned Handle() const noexcept { return id; }
	[[nodiscard]] bool IsValid() const noexcept { return id > 0; }

	uint32_t id{ 0 };
};
//=============================================================================
gpu::texture::TexturePtr gpu::texture::CreateTexture(const TextureCreateInfo& createInfo, std::string_view name)
{
	auto texture = std::make_shared<Texture>();
	texture->createInfo = createInfo;

	glCreateTextures(EnumToValue(createInfo.imageType), 1, &texture->id);

	GLint glFormat = EnumToValue(createInfo.format);

	switch (createInfo.imageType)
	{
	case ImageType::Texture1D:
		glTextureStorage1D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width);
		break;
	case ImageType::Texture2D:
		glTextureStorage2D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width, createInfo.extent.height);
		break;
	case ImageType::Texture3D:
		glTextureStorage3D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width, createInfo.extent.height, createInfo.extent.depth);
		break;
	case ImageType::Texture1DArray:
		glTextureStorage2D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width, createInfo.arrayLayers);
		break;
	case ImageType::Texture2DArray:
		glTextureStorage3D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width, createInfo.extent.height, createInfo.arrayLayers);
		break;
	case ImageType::TextureCubemap:
		glTextureStorage2D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width, createInfo.extent.height);
		break;
	case ImageType::TextureCubemapArray:
		glTextureStorage3D(texture->id, createInfo.mipLevels, glFormat, createInfo.extent.width, createInfo.extent.height, createInfo.arrayLayers);
		break;
	case ImageType::Texture2DMultisample:
		glTextureStorage2DMultisample(texture->id, EnumToValue(createInfo.sampleCount), glFormat, createInfo.extent.width, createInfo.extent.height, GL_TRUE);
		break;
	case ImageType::Texture2DMultisampleArray:
		glTextureStorage3DMultisample(texture->id, EnumToValue(createInfo.sampleCount), glFormat, createInfo.extent.width, createInfo.extent.height, createInfo.arrayLayers, GL_TRUE);
		break;
	default: std::unreachable(); break;
	}

	if (!name.empty())
		glObjectLabel(GL_TEXTURE, texture->id, static_cast<GLsizei>(name.length()), name.data());

	core::Debug("Created texture with handle " + std::to_string(texture->id));

	return texture;
}
//=============================================================================
gpu::texture::TexturePtr gpu::texture::CreateTexture2D(core::Extent2D size, Format format, std::string_view name)
{
	TextureCreateInfo createInfo{
		.imageType   = ImageType::Texture2D,
		.format      = format,
		.extent      = {size.width, size.height, 1},
		.mipLevels   = 1,
		.arrayLayers = 1,
		.sampleCount = SampleCount::Samples1,
	};
	return CreateTexture(createInfo, name);
}
//=============================================================================
gpu::texture::TexturePtr gpu::texture::CreateTexture2DMip(core::Extent2D size, Format format, uint32_t mipLevels, std::string_view name)
{
	TextureCreateInfo createInfo{
		.imageType   = ImageType::Texture2D,
		.format      = format,
		.extent      = {size.width, size.height, 1},
		.mipLevels   = mipLevels,
		.arrayLayers = 1,
		.sampleCount = SampleCount::Samples1,
	};
	return CreateTexture(createInfo, name);
}
//=============================================================================
gpu::texture::TexturePtr gpu::texture::LoadTexture2D(std::string_view path)
{
	int x = 0;
	int y = 0;
	const auto imageData = stbi_load(path.data(), &x, &y, nullptr, 4);
	assert(imageData); // TODO:
	auto texture = CreateTexture2D({ static_cast<uint32_t>(x), static_cast<uint32_t>(y) }, Format::R8G8B8A8_UNORM);
	UpdateImage(texture, {
	  .extent = {static_cast<uint32_t>(x), static_cast<uint32_t>(y)},
	  .format = UploadFormat::RGBA,
	  .type = UploadType::UBYTE,
	  .pixels = imageData,
		});
	stbi_image_free(imageData);

	return texture;
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateTextureView(const TextureViewCreateInfo& viewInfo, TexturePtr texture, std::string_view name)
{
	auto view = std::make_shared<TextureView>();
	view->viewInfo = viewInfo;
	view->createInfo = texture->createInfo;

	glGenTextures(1, &view->id); // glCreateTextures does not work here
	glTextureView(view->id,
		EnumToValue(viewInfo.viewType),
		texture->id,
		EnumToValue(viewInfo.format),
		viewInfo.minLevel,
		viewInfo.numLevels,
		viewInfo.minLayer,
		viewInfo.numLayers);

	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_R, EnumToValue(viewInfo.components.r));
	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_G, EnumToValue(viewInfo.components.g));
	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_B, EnumToValue(viewInfo.components.b));
	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_A, EnumToValue(viewInfo.components.a));

	if (!name.empty())
		glObjectLabel(GL_TEXTURE, view->id, static_cast<GLsizei>(name.length()), name.data());

	core::Debug("Created texture view with handle " + std::to_string(view->id));

	return view;
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateTextureView(const TextureViewCreateInfo& viewInfo, TextureViewPtr textureView, std::string_view name)
{
	auto view = std::make_shared<TextureView>();
	view->viewInfo = viewInfo;
	view->createInfo = TextureCreateInfo{
		.imageType = textureView->viewInfo.viewType,
		.format = textureView->viewInfo.format,
		.extent = textureView->createInfo.extent,
		.mipLevels = textureView->viewInfo.numLevels,
		.arrayLayers = textureView->viewInfo.numLayers,
	};

	glGenTextures(1, &view->id); // glCreateTextures does not work here
	glTextureView(view->id,
		EnumToValue(viewInfo.viewType),
		textureView->id,
		EnumToValue(viewInfo.format),
		viewInfo.minLevel,
		viewInfo.numLevels,
		viewInfo.minLayer,
		viewInfo.numLayers);

	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_R, EnumToValue(viewInfo.components.r));
	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_G, EnumToValue(viewInfo.components.g));
	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_B, EnumToValue(viewInfo.components.b));
	glTextureParameteri(view->id, GL_TEXTURE_SWIZZLE_A, EnumToValue(viewInfo.components.a));

	if (!name.empty())
		glObjectLabel(GL_TEXTURE, view->id, static_cast<GLsizei>(name.length()), name.data());

	core::Debug("Created texture view with handle " + std::to_string(view->id));

	return view;
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateTextureView(TexturePtr texture, std::string_view name)
{
	auto tvci = TextureViewCreateInfo{
		.viewType = texture->createInfo.imageType,
		.format = texture->createInfo.format,
		.minLevel = 0,
		.numLevels = texture->createInfo.mipLevels,
		.minLayer = 0,
		.numLayers = texture->createInfo.arrayLayers,
	};
	return CreateTextureView(tvci, texture, name);
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateSingleMipView(TexturePtr texture, uint32_t level)
{
	TextureViewCreateInfo createInfo{
		.viewType = texture->createInfo.imageType,
		.format = texture->createInfo.format,
		.minLevel = level,
		.numLevels = 1,
		.minLayer = 0,
		.numLayers = texture->createInfo.arrayLayers,
	};
	return CreateTextureView(createInfo, texture);
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateSingleLayerView(TexturePtr texture, uint32_t layer)
{
	TextureViewCreateInfo createInfo{
		.viewType = texture->createInfo.imageType,
		.format = texture->createInfo.format,
		.minLevel = 0,
		.numLevels = texture->createInfo.mipLevels,
		.minLayer = layer,
		.numLayers = 1,
	};
	return CreateTextureView(createInfo, texture);
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateFormatView(TexturePtr texture, Format newFormat)
{
	TextureViewCreateInfo createInfo{
		.viewType = texture->createInfo.imageType,
		.format = newFormat,
		.minLevel = 0,
		.numLevels = texture->createInfo.mipLevels,
		.minLayer = 0,
		.numLayers = texture->createInfo.arrayLayers,
	};
	return CreateTextureView(createInfo, texture);
}
//=============================================================================
gpu::texture::TextureViewPtr gpu::texture::CreateSwizzleView(TexturePtr texture, ComponentMapping components)
{
	TextureViewCreateInfo createInfo{
		.viewType = texture->createInfo.imageType,
		.format = texture->createInfo.format,
		.components = components,
		.minLevel = 0,
		.numLevels = texture->createInfo.mipLevels,
		.minLayer = 0,
		.numLayers = texture->createInfo.arrayLayers,
	};
	return CreateTextureView(createInfo, texture);
}
//=============================================================================
gpu::texture::SamplerPtr gpu::texture::CreateSampler(const SamplerState& samplerState)
{
	if (auto it = context.samplerCache.find(samplerState); it != context.samplerCache.end())
	{
		return it->second;
	}

	auto sampler = std::make_shared<Sampler>();

	glSamplerParameteri(sampler->id,
		GL_TEXTURE_COMPARE_MODE,
		samplerState.compareEnable ? GL_COMPARE_REF_TO_TEXTURE : GL_NONE);

	glSamplerParameteri(sampler->id, GL_TEXTURE_COMPARE_FUNC, EnumToValue(samplerState.compareOp));

	GLint magFilter = samplerState.magFilter == Filter::Linear ? GL_LINEAR : GL_NEAREST;
	glSamplerParameteri(sampler->id, GL_TEXTURE_MAG_FILTER, magFilter);

	GLint minFilter{};
	switch (samplerState.mipmapFilter)
	{
	case Filter::None:
		minFilter = samplerState.minFilter == Filter::Linear ? GL_LINEAR : GL_NEAREST;
		break;
	case Filter::Nearest:
		minFilter = samplerState.minFilter == Filter::Linear ? GL_LINEAR_MIPMAP_NEAREST : GL_NEAREST_MIPMAP_NEAREST;
		break;
	case Filter::Linear:
		minFilter = samplerState.minFilter == Filter::Linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_LINEAR;
		break;
	default: std::unreachable();
	}
	glSamplerParameteri(sampler->id, GL_TEXTURE_MIN_FILTER, minFilter);

	glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_S, EnumToValue(samplerState.addressModeU));
	glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_T, EnumToValue(samplerState.addressModeV));
	glSamplerParameteri(sampler->id, GL_TEXTURE_WRAP_R, EnumToValue(samplerState.addressModeW));

	// TODO: determine whether int white values should be 1 or 255
	switch (samplerState.borderColor)
	{
	case BorderColor::FloatTransparentBlack:
	{
		constexpr GLfloat color[4]{ 0, 0, 0, 0 };
		glSamplerParameterfv(sampler->id, GL_TEXTURE_BORDER_COLOR, color);
		break;
	}
	case BorderColor::IntTransparentBlack:
	{
		constexpr GLint color[4]{ 0, 0, 0, 0 };
		glSamplerParameteriv(sampler->id, GL_TEXTURE_BORDER_COLOR, color);
		break;
	}
	case BorderColor::FloatOpaqueBlack:
	{
		constexpr GLfloat color[4]{ 0, 0, 0, 1 };
		glSamplerParameterfv(sampler->id, GL_TEXTURE_BORDER_COLOR, color);
		break;
	}
	case BorderColor::IntOpaqueBlack:
	{
		// constexpr GLint color[4]{ 0, 0, 0, 255 };
		constexpr GLint color[4]{ 0, 0, 0, 1 };
		glSamplerParameteriv(sampler->id, GL_TEXTURE_BORDER_COLOR, color);
		break;
	}
	case BorderColor::FloatOpaqueWhite:
	{
		constexpr GLfloat color[4]{ 1, 1, 1, 1 };
		glSamplerParameterfv(sampler->id, GL_TEXTURE_BORDER_COLOR, color);
		break;
	}
	case BorderColor::IntOpaqueWhite:
	{
		// constexpr GLint color[4]{ 255, 255, 255, 255 };
		constexpr GLint color[4]{ 1, 1, 1, 1 };
		glSamplerParameteriv(sampler->id, GL_TEXTURE_BORDER_COLOR, color);
		break;
	}
	default: std::unreachable(); break;
	}

	glSamplerParameterf(sampler->id,
		GL_TEXTURE_MAX_ANISOTROPY,
		static_cast<GLfloat>(EnumToValue(samplerState.anisotropy)));

	glSamplerParameterf(sampler->id, GL_TEXTURE_LOD_BIAS, samplerState.lodBias);

	glSamplerParameterf(sampler->id, GL_TEXTURE_MIN_LOD, samplerState.minLod);

	glSamplerParameterf(sampler->id, GL_TEXTURE_MAX_LOD, samplerState.maxLod);

	core::Debug("Created sampler with handle " + std::to_string(sampler->id));

	return context.samplerCache.insert({ samplerState, sampler }).first->second;
}
//=============================================================================
uint64_t gpu::texture::GetBindlessHandle(TexturePtr texture, Sampler sampler)
{
	assert(texture->bindlessHandle == 0 && "Texture already has bindless handle resident.");
	texture->bindlessHandle = glGetTextureSamplerHandleARB(texture->id, sampler.Handle());
	assert(texture->bindlessHandle != 0 && "Failed to create texture sampler handle.");
	glMakeTextureHandleResidentARB(texture->bindlessHandle);
	return texture->bindlessHandle;
}
//=============================================================================
void gpu::texture::GenMipmaps(TexturePtr texture)
{
	assert(texture);
	glGenerateTextureMipmap(texture->id);
}
//=============================================================================
const gpu::texture::TextureCreateInfo& gpu::texture::GetCreateInfo(TexturePtr texture) noexcept
{
	return texture ? texture->createInfo : TextureCreateInfo{};
}
//=============================================================================
core::Extent3D gpu::texture::Extent(TexturePtr texture) noexcept
{
	return texture ? texture->Extent() : core::Extent3D{};
}
//=============================================================================
uint32_t gpu::texture::Handle(TexturePtr texture) noexcept
{
	return texture ? texture->Handle() : 0;
}
//=============================================================================
bool gpu::texture::IsValid(TexturePtr texture) noexcept
{
	return texture ? texture->IsValid() : false;
}
//=============================================================================
const gpu::texture::TextureCreateInfo& gpu::texture::GetCreateInfo(TextureViewPtr view) noexcept
{
	return view ? view->createInfo : TextureCreateInfo{};
}
//=============================================================================
const gpu::texture::TextureViewCreateInfo& gpu::texture::GetViewInfo(TextureViewPtr view) noexcept
{
	return view ? view->viewInfo : TextureViewCreateInfo{};
}
//=============================================================================
core::Extent3D gpu::texture::Extent(TextureViewPtr view) noexcept
{
	return view ? view->Extent() : core::Extent3D{};
}
//=============================================================================
uint32_t gpu::texture::Handle(TextureViewPtr view) noexcept
{
	return view ? view->Handle() : 0;
}
//=============================================================================
bool gpu::texture::IsValid(TextureViewPtr view) noexcept
{
	return view ? view->IsValid() : false;
}
//=============================================================================
uint32_t gpu::texture::Handle(SamplerPtr sampler) noexcept
{
	return sampler ? sampler->Handle() : 0;
}
//=============================================================================
bool gpu::texture::IsValid(SamplerPtr sampler) noexcept
{
	return sampler ? sampler->IsValid() : false;
}
//=============================================================================
void gpu::texture::UpdateImage(TexturePtr texture, const TextureUpdateInfo& info)
{
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

	assert(!IsBlockCompressedFormat(texture->createInfo.format));
	GLenum format{};
	if (info.format == UploadFormat::INFER_FORMAT)
		format = EnumToValue(FormatToUploadFormat(texture->createInfo.format));
	else
		format = EnumToValue(info.format);

	GLenum type{};
	if (info.type == UploadType::INFER_TYPE)
		type = FormatToTypeGL(texture->createInfo.format);
	else
		type = EnumToValue(info.type);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, info.rowLength);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, info.imageHeight);

	switch (ImageTypeToDimension(texture->createInfo.imageType))
	{
	case 1:
		glTextureSubImage1D(texture->id, info.level, info.offset.x, info.extent.width, format, type, info.pixels); break;
	case 2:
		glTextureSubImage2D(texture->id, info.level, info.offset.x, info.offset.y, info.extent.width, info.extent.height, format, type, info.pixels);
		break;
	case 3:
		glTextureSubImage3D(texture->id, info.level, info.offset.x, info.offset.y, info.offset.z, info.extent.width, info.extent.height, info.extent.depth, format, type, info.pixels);
		break;
	}
}
//=============================================================================
void gpu::texture::UpdateCompressedImage(TexturePtr texture, const CompressedTextureUpdateInfo& info)
{
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	assert(IsBlockCompressedFormat(texture->createInfo.format));
	const GLenum format = EnumToValue(texture->createInfo.format);

	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

	switch (ImageTypeToDimension(texture->createInfo.imageType))
	{
	case 2:
		glCompressedTextureSubImage2D(
			texture->id,
			info.level,
			info.offset.x,
			info.offset.y,
			info.extent.width,
			info.extent.height,
			format,
			static_cast<uint32_t>(getBlockCompressedImageSize(texture->createInfo.format, info.extent.width, info.extent.height, 1)),
			info.data);
		break;
	case 3:
		glCompressedTextureSubImage3D(
			texture->id,
			info.level,
			info.offset.x,
			info.offset.y,
			info.offset.z,
			info.extent.width,
			info.extent.height,
			info.extent.depth,
			format,
			static_cast<uint32_t>(getBlockCompressedImageSize(texture->createInfo.format, info.extent.width, info.extent.height, info.extent.depth)),
			info.data);
		break;
	default: std::unreachable();
	}
}
//=============================================================================
void gpu::texture::ClearImage(TexturePtr texture, const TextureClearInfo& info)
{
	// Infer format
	GLenum format{};
	if (info.format == UploadFormat::INFER_FORMAT)
		format = EnumToValue(FormatToUploadFormat(texture->createInfo.format));
	else
		format = EnumToValue(info.format);

	// Infer type
	GLenum type{};
	if (info.type == UploadType::INFER_TYPE)
		type = FormatToTypeGL(texture->createInfo.format);
	else
		type = EnumToValue(info.type);

	// Infer extent
	core::Extent3D extent = info.extent;
	if (extent == core::Extent3D{})
	{
		extent = texture->createInfo.extent >> info.level;
		extent.width = std::max(extent.width, 1u);
		extent.height = std::max(extent.height, 1u);
		extent.depth = std::max(extent.depth, 1u);
	}

	glClearTexSubImage(texture->id, info.level,
		info.offset.x, info.offset.y, info.offset.z,
		extent.width, extent.height, extent.depth,
		format, type, info.data);
}
//=============================================================================