#include "stdafx.h"
#include "assets.hpp"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <unordered_map>
//=============================================================================
namespace fs = std::filesystem;
//=============================================================================
namespace
{
	// Split a string by a delimiter character
	static std::vector<std::string> splitString(const std::string& s, char delim)
	{
		std::vector<std::string> result;
		std::string token;
		std::istringstream ss(s);
		while (std::getline(ss, token, delim))
		{
			if (!token.empty())
				result.push_back(token);
		}
		return result;
	}

	// Load an OBJ file and return meshes (supports usemtl primary/secondary submeshes)
	static std::vector<gr::Mesh> LoadOBJFromStream(std::istream& stream)
	{
		struct VertexData
		{
			glm::vec3 pos;
			glm::vec2 uv;
			glm::vec3 norm;
		};

		struct SubMeshData
		{
			std::vector<VertexData> verts;
			std::vector<uint32_t> indices;
			std::unordered_map<std::string, uint32_t> mapper;
		};

		std::vector<SubMeshData> subMeshes(1);
		int currentMesh = 0;

		std::vector<glm::vec3> positions;
		std::vector<glm::vec2> uvs;
		std::vector<glm::vec3> normals;

		positions.reserve(256);
		uvs.reserve(256);
		normals.reserve(256);

		std::string line;
		while (std::getline(stream, line))
		{
			auto tokens = splitString(line, ' ');
			if (tokens.empty()) continue;

			if (tokens[0] == "v" && tokens.size() >= 4)
			{
				positions.push_back(glm::vec3(
					std::stof(tokens[1]),
					std::stof(tokens[2]),
					std::stof(tokens[3])));
			}
			else if (tokens[0] == "vt" && tokens.size() >= 3)
			{
				uvs.push_back(glm::vec2(
					std::stof(tokens[1]),
					1.0f - std::stof(tokens[2]))); // flip Y
			}
			else if (tokens[0] == "vn" && tokens.size() >= 4)
			{
				normals.push_back(glm::vec3(
					std::stof(tokens[1]),
					std::stof(tokens[2]),
					std::stof(tokens[3])));
			}
			else if (tokens[0] == "usemtl" && tokens.size() > 1)
			{
				std::string mtlName = tokens[1];
				std::transform(mtlName.begin(), mtlName.end(), mtlName.begin(),
					[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
				currentMesh = (mtlName == "secondary") ? 1 : 0;
				while ((size_t)currentMesh >= subMeshes.size())
					subMeshes.emplace_back();
			}
			else if (tokens[0] == "f" && tokens.size() >= 4)
			{
				auto& mesh = subMeshes[currentMesh];
				std::vector<uint32_t> faceVerts;
				for (size_t i = 1; i < tokens.size(); ++i)
				{
					auto idxParts = splitString(tokens[i], '/');
					if (idxParts.size() < 3) continue;

					int posIdx = std::stoi(idxParts[0]) - 1;
					int uvIdx  = std::stoi(idxParts[1]) - 1;
					int nrmIdx = std::stoi(idxParts[2]) - 1;

					const std::string& key = tokens[i];
					auto it = mesh.mapper.find(key);
					if (it != mesh.mapper.end())
					{
						faceVerts.push_back(it->second);
					}
					else
					{
						uint32_t newIdx = static_cast<uint32_t>(mesh.verts.size());
						VertexData vd;
						vd.pos  = (posIdx >= 0 && (size_t)posIdx < positions.size())
							? positions[posIdx] : glm::vec3(0.0f);
						vd.uv   = (uvIdx >= 0 && (size_t)uvIdx < uvs.size())
							? uvs[uvIdx] : glm::vec2(0.0f);
						vd.norm = (nrmIdx >= 0 && (size_t)nrmIdx < normals.size())
							? normals[nrmIdx] : glm::vec3(0.0f, 1.0f, 0.0f);
						mesh.verts.push_back(vd);
						mesh.mapper[key] = newIdx;
						faceVerts.push_back(newIdx);
					}
				}
				// Fan-triangulate preserving original OBJ winding order
				if (faceVerts.size() >= 3)
				{
					for (size_t i = 2; i < faceVerts.size(); ++i)
					{
						mesh.indices.push_back(faceVerts[0]);
						mesh.indices.push_back(faceVerts[i - 1]);
						mesh.indices.push_back(faceVerts[i]);
					}
				}
			}
			// ignore o, mtllib, s, etc.
		}

		// Build GPU meshes
		std::vector<gr::Mesh> result;
		for (auto& sub : subMeshes)
		{
			if (sub.verts.empty())
				continue;

			std::vector<gr::MeshVertex> meshVerts;
			meshVerts.reserve(sub.verts.size());
			std::vector<glm::vec3> aabbPositions;
			aabbPositions.reserve(sub.verts.size());

			for (const auto& vd : sub.verts)
			{
				meshVerts.push_back(gr::MeshVertex{
					.position = vd.pos,
					.normal   = vd.norm,
					.uv       = vd.uv,
					.color    = glm::vec4(1.0f)
				});
				aabbPositions.push_back(vd.pos);
			}

			gr::Mesh mesh;
			mesh.vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);
			mesh.vbo = gpu::buffer::CreateBuffer(
				meshVerts.data(), meshVerts.size() * sizeof(gr::MeshVertex));
			mesh.ibo = gpu::buffer::CreateBuffer(
				sub.indices.data(), sub.indices.size() * sizeof(uint32_t));
			mesh.vertexCount = static_cast<uint32_t>(meshVerts.size());
			mesh.indexCount  = static_cast<uint32_t>(sub.indices.size());
			mesh.isIndexed   = true;
			mesh.ComputeAABB(aabbPositions);

			result.push_back(std::move(mesh));
		}

		if (result.empty())
			result.push_back(gr::Mesh::CreateCube());

		return result;
	}
} // anonymous namespace
//=============================================================================
void ed::Assets::Init(const fs::path& texturesDir, const fs::path& shapesDir)
{
	auto& inst = Get();
	if (inst._initialized)
		return;

	// Pre-built meshes
	inst._entSphere = std::make_shared<gr::Mesh>(gr::Mesh::CreateSphere(16, 16));
	inst._spriteQuad = std::make_shared<gr::Mesh>(gr::Mesh::CreateQuad());

	inst.discoverTextures(texturesDir);
	inst.discoverModels(shapesDir);

	inst._initialized = true;
}
//=============================================================================
void ed::Assets::ReloadTexturePaths(const fs::path& texturesDir)
{
	Get().discoverTextures(texturesDir);
}
//=============================================================================
void ed::Assets::ReloadModelPaths(const fs::path& shapesDir)
{
	Get().discoverModels(shapesDir);
}
//=============================================================================
void ed::Assets::discoverTextures(const fs::path& dir)
{
	_texturePaths.clear();
	if (!fs::exists(dir)) return;
	for (const auto& entry : fs::directory_iterator(dir))
	{
		if (entry.is_regular_file())
		{
			auto ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp" || ext == ".tga")
				_texturePaths.push_back(entry.path());
		}
	}
}
//=============================================================================
void ed::Assets::discoverModels(const fs::path& dir)
{
	_modelPaths.clear();
	if (!fs::exists(dir)) return;
	for (const auto& entry : fs::directory_iterator(dir))
	{
		if (entry.is_regular_file())
		{
			auto ext = entry.path().extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(),
				[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
			if (ext == ".obj" || ext == ".gltf" || ext == ".glb")
				_modelPaths.push_back(entry.path());
		}
	}
}
//=============================================================================
std::shared_ptr<ed::TexHandle> ed::Assets::GetTexture(const fs::path& path)
{
	auto& inst = Get();
	auto it = inst._textureCache.find(path);
	if (it != inst._textureCache.end())
	{
		auto ptr = it->second.lock();
		if (ptr) return ptr;
	}

	auto tex = gpu::texture::LoadTexture2D(path.generic_string());
	if (!gpu::texture::IsValid(tex))
	{
		tex = GetMissingTexture();
	}

	auto handle = std::make_shared<TexHandle>(path, tex);
	inst._textureCache[path] = handle;
	return handle;
}
//=============================================================================
	std::shared_ptr<ed::ModelHandle> ed::Assets::GetModel(const fs::path& path)
{
	auto& inst = Get();
	auto it = inst._modelCache.find(path);
	if (it != inst._modelCache.end())
	{
		auto ptr = it->second.lock();
		if (ptr) return ptr;
	}

	// Try loading OBJ file from disk
	std::vector<std::shared_ptr<gr::Mesh>> meshes;
	if (fs::exists(path))
	{
		std::ifstream file(path);
		if (file.is_open())
		{
			try
			{
				auto loaded = LoadOBJFromStream(file);
				for (auto& m : loaded)
					meshes.push_back(std::make_shared<gr::Mesh>(std::move(m)));
			}
			catch (...)
			{
				meshes.push_back(std::make_shared<gr::Mesh>(gr::Mesh::CreateCube()));
			}
		}
		else
		{
			meshes.push_back(std::make_shared<gr::Mesh>(gr::Mesh::CreateCube()));
		}
	}
	else
	{
		meshes.push_back(std::make_shared<gr::Mesh>(gr::Mesh::CreateCube()));
	}

	auto handle = std::make_shared<ModelHandle>(path, std::move(meshes));
	inst._modelCache[path] = handle;
	return handle;
}
//=============================================================================
std::shared_ptr<gr::Mesh> ed::Assets::GetEntSphere() noexcept
{
	return Get()._entSphere;
}
//=============================================================================
std::shared_ptr<gr::Mesh> ed::Assets::GetSpriteQuad() noexcept
{
	return Get()._spriteQuad;
}
//=============================================================================
ImFont* ed::Assets::GetUIFont(float size)
{
	auto& inst = Get();
	ImGuiIO& io = ImGui::GetIO();
	ImFont* font = io.Fonts->AddFontFromFileTTF("data/fonts/segoeui.ttf", size);
	if (font)
		inst._uiFontLarge = font;
	return inst._uiFontLarge ? inst._uiFontLarge : io.Fonts->Fonts[0];
}
//=============================================================================
ImFont* ed::Assets::GetCodeFont() noexcept
{
	auto& inst = Get();
	if (!inst._codeFont)
	{
		ImGuiIO& io = ImGui::GetIO();
		inst._codeFont = io.Fonts->AddFontFromFileTTF("data/fonts/cour.ttf", 14.0f);
	}
	return inst._codeFont;
}
//=============================================================================
gpu::texture::TexturePtr ed::Assets::GetMissingTexture()
{
	static auto tex = gpu::texture::CreateTexture2D(
		{ 4, 4 }, gpu::Format::R8G8B8A8_UNORM, "missing_tex");
	// Fill with magenta/black checkerboard
	std::array<uint32_t, 16> pixels;
	for (int y = 0; y < 4; ++y)
		for (int x = 0; x < 4; ++x)
			pixels[y * 4 + x] = ((x ^ y) & 1) ? 0xFFFF00FF : 0xFF000000;
	gpu::texture::TextureUpdateInfo update{};
	update.level = 0;
	update.extent = { 4, 4, 1 };
	update.pixels = pixels.data();
	update.format = gpu::UploadFormat::RGBA;
	update.type = gpu::UploadType::UBYTE;
	gpu::texture::UpdateImage(tex, update);
	return tex;
}
//=============================================================================
std::shared_ptr<gr::Mesh> ed::Assets::GetMissingModel()
{
	static auto mesh = std::make_shared<gr::Mesh>(gr::Mesh::CreateCube());
	return mesh;
}
//=============================================================================