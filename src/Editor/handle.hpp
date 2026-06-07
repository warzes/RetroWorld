#pragma once

#include <memory>
#include <vector>
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
		: _path(path)
	{
		_meshes.push_back(std::move(mesh));
	}

	ModelHandle(const std::filesystem::path& path, std::vector<std::shared_ptr<gr::Mesh>> meshes)
		: _path(path), _meshes(std::move(meshes))
	{}

	[[nodiscard]] const std::filesystem::path& GetPath() const noexcept { return _path; }
	[[nodiscard]] std::shared_ptr<gr::Mesh> GetMesh(size_t index = 0) const noexcept
	{
		return (index < _meshes.size()) ? _meshes[index] : nullptr;
	}
	[[nodiscard]] size_t GetMeshCount() const noexcept { return _meshes.size(); }

private:
	std::filesystem::path _path;
	std::vector<std::shared_ptr<gr::Mesh>> _meshes;
};
} // namespace ed