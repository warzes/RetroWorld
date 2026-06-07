#pragma once

#include <memory>
#include <string>
#include <filesystem>
#include <gpu_texture.h>
#include <gr_mesh.h>

namespace ed
{
	class TexHandle final
	{
	public:
		TexHandle(const std::filesystem::path& path, gpu::texture::TexturePtr tex)
			: _path(path), _texture(std::move(tex))
		{}

		[[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return _path; }
		[[nodiscard]] gpu::texture::TexturePtr GetTexture() const noexcept { return _texture; }

	private:
		std::filesystem::path _path;
		gpu::texture::TexturePtr _texture;
	};

	class ModelHandle final
	{
	public:
		ModelHandle(const std::filesystem::path& path, std::shared_ptr<gr::Mesh> mesh)
			: _path(path), _mesh(std::move(mesh))
		{}

		[[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return _path; }
		[[nodiscard]] std::shared_ptr<gr::Mesh> GetMesh() const noexcept { return _mesh; }

	private:
		std::filesystem::path _path;
		std::shared_ptr<gr::Mesh> _mesh;
	};
} // namespace ed