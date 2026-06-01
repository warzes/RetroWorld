#include "stdafx.h"
#include "Decorations.h"
#include "Editor.h"

//=============================================================================
namespace
{
	using namespace decorations;

	struct ModelCacheEntry final
	{
		CachedModel cached;
		bool        loaded = false;
	};

	std::unordered_map<std::string, ModelCacheEntry> s_cache;
	std::shared_ptr<gr::Material> s_defaultMaterial;
	std::shared_ptr<gr::Material> s_previewMaterial;

	constexpr const char* DECORATIONS_ROOT = "data/decorations";
} // anonymous namespace

//=============================================================================
std::vector<std::string> decorations::ScanFolders()
{
	std::vector<std::string> result;
	for (auto& entry : std::filesystem::directory_iterator(DECORATIONS_ROOT))
	{
		if (entry.is_directory())
			result.push_back(entry.path().filename().string());
	}
	std::sort(result.begin(), result.end());
	return result;
}

//=============================================================================
std::vector<std::string> decorations::ScanModels(const std::string& folder)
{
	std::vector<std::string> result;
	std::string dir = std::string(DECORATIONS_ROOT) + "/" + folder;
	if (!std::filesystem::exists(dir))
		return result;

	for (auto& entry : std::filesystem::directory_iterator(dir))
	{
		if (entry.is_regular_file())
		{
			auto ext = entry.path().extension().string();
			if (ext == ".obj" || ext == ".OBJ")
				result.push_back(entry.path().filename().string());
		}
	}
	std::sort(result.begin(), result.end());
	return result;
}

//=============================================================================
std::string decorations::ModelKey(const std::string& folder, const std::string& modelFile)
{
	return folder + "/" + modelFile;
}

//=============================================================================
bool decorations::EnsureModelLoaded(const std::string& folder, const std::string& modelFile)
{
	std::string key = ModelKey(folder, modelFile);
	auto it = s_cache.find(key);
	if (it != s_cache.end())
		return it->second.loaded;

	ModelCacheEntry entry;
	std::string filepath = std::string(DECORATIONS_ROOT) + "/" + key;

	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(filepath))
	{
		if (!reader.Error().empty())
			core::Warning("OBJ error [" + filepath + "]: " + reader.Error());
		entry.loaded = false;
		s_cache[key] = std::move(entry);
		return false;
	}

	if (!reader.Warning().empty())
		core::Warning("OBJ warning [" + filepath + "]: " + reader.Warning());

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();

	if (shapes.empty())
	{
		core::Warning("OBJ has no shapes: " + filepath);
		entry.loaded = false;
		s_cache[key] = std::move(entry);
		return false;
	}

	// Convert tinyobj data to MeshVertex format
	std::vector<gr::MeshVertex> vertices;
	std::vector<uint32_t> indices;
	vertices.reserve(1024);
	indices.reserve(4096);

	for (const auto& shape : shapes)
	{
		for (const auto& idx : shape.mesh.indices)
		{
			glm::vec3 pos{ 0.0f };
			if (idx.vertex_index >= 0)
			{
				pos.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				pos.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				pos.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];
			}

			glm::vec3 normal{ 0.0f, 1.0f, 0.0f };
			if (idx.normal_index >= 0)
			{
				normal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
				normal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
				normal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
			}

			glm::vec2 uv{ 0.0f };
			if (idx.texcoord_index >= 0)
			{
				uv.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
				uv.y = 1.0f - attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
			}

			vertices.push_back({ pos, normal, uv, { 1.0f, 1.0f, 1.0f, 1.0f } });
			indices.push_back(static_cast<uint32_t>(indices.size()));
		}
	}

	if (vertices.empty())
	{
		entry.loaded = false;
		s_cache[key] = std::move(entry);
		return false;
	}

	// Create VAO
	auto vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);

	// Create VBO
	auto vbo = gpu::buffer::CreateBuffer(
		vertices.data(),
		vertices.size() * sizeof(gr::MeshVertex));

	// Create IBO
	auto ibo = gpu::buffer::CreateBuffer(
		indices.data(),
		indices.size() * sizeof(uint32_t));

	auto mesh = std::make_shared<gr::Mesh>();
	mesh->vao = std::move(vao);
	mesh->vbo = std::move(vbo);
	mesh->ibo = std::move(ibo);
	mesh->vertexCount = static_cast<uint32_t>(vertices.size());
	mesh->indexCount = static_cast<uint32_t>(indices.size());
	mesh->isIndexed = true;

	std::vector<glm::vec3> positions(vertices.size());
	for (size_t i = 0; i < vertices.size(); ++i)
		positions[i] = vertices[i].position;
	mesh->ComputeAABB(positions);

	entry.cached.aabb = mesh->aabb;
	entry.cached.mesh = std::move(mesh);
	entry.cached.material = GetDefaultMaterial();
	entry.loaded = true;
	s_cache[key] = std::move(entry);
	return true;
}

//=============================================================================
decorations::CachedModel* decorations::GetCachedModel(const std::string& folder, const std::string& modelFile)
{
	std::string key = ModelKey(folder, modelFile);
	auto it = s_cache.find(key);
	if (it != s_cache.end() && it->second.loaded)
		return &it->second.cached;
	return nullptr;
}

//=============================================================================
std::shared_ptr<gr::Material> decorations::GetDefaultMaterial()
{
	if (!s_defaultMaterial)
	{
		s_defaultMaterial = std::make_shared<gr::Material>();
		s_defaultMaterial->albedoColor   = { 0.8f, 0.8f, 0.8f };
		s_defaultMaterial->specularColor = { 0.2f, 0.2f, 0.2f };
		s_defaultMaterial->ambientColor  = { 0.1f, 0.1f, 0.1f };
		s_defaultMaterial->shininess     = 16.0f;
		s_defaultMaterial->opacity       = 1.0f;
	}
	return s_defaultMaterial;
}

//=============================================================================
std::shared_ptr<gr::Material> decorations::GetPreviewMaterial()
{
	if (!s_previewMaterial)
	{
		s_previewMaterial = std::make_shared<gr::Material>(*GetDefaultMaterial());
		s_previewMaterial->opacity = 0.4f;
	}
	return s_previewMaterial;
}

//=============================================================================
scene::ModelNode* decorations::CreateSceneNode(const std::string& folder, const std::string& modelFile, const glm::vec3& pos)
{
	auto* cached = GetCachedModel(folder, modelFile);
	if (!cached || !cached->mesh)
		return nullptr;

	auto* decoParent = g_scene->root->FindChild("decorations");
	if (!decoParent)
		decoParent = &g_scene->root->AddChild<scene::SceneNode>("decorations");

	auto& node = decoParent->AddChild<scene::ModelNode>("deco_" + ModelKey(folder, modelFile));
	node.mesh = cached->mesh;
	node.material = cached->material;
	node.transform.position = pos;
	return &node;
}

//=============================================================================
void decorations::DestroySceneNode(int index)
{
	if (index < 0 || index >= static_cast<int>(g_decorations.size()))
		return;

	auto* decoParent = g_scene->root->FindChild("decorations");
	if (!decoParent)
		return;

	std::string targetName = "deco_" + ModelKey(g_decorations[index].folder, g_decorations[index].modelFile);

	// Remove children in reverse order since removing shifts indices
	for (int i = static_cast<int>(decoParent->children.size()) - 1; i >= 0; --i)
	{
		if (decoParent->children[i]->name == targetName)
		{
			decoParent->children.erase(decoParent->children.begin() + i);
			return;
		}
	}
}

//=============================================================================
void decorations::UpdateSceneTransform(int index)
{
	if (index < 0 || index >= static_cast<int>(g_decorations.size()))
		return;

	auto& inst = g_decorations[index];
	auto* decoParent = g_scene->root->FindChild("decorations");
	if (!decoParent)
		return;

	std::string targetName = "deco_" + ModelKey(inst.folder, inst.modelFile);
	for (auto& child : decoParent->children)
	{
		if (child->name == targetName)
		{
			child->transform.position = inst.position;
			child->transform.rotation = glm::quat(glm::radians(inst.rotation));
			child->transform.scale    = inst.scale;
			return;
		}
	}
}

//=============================================================================
void decorations::RebuildAllSceneNodes()
{
	// Remove old decorations node
	auto* decoParent = g_scene->root->FindChild("decorations");
	if (decoParent)
	{
		g_scene->root->RemoveChild("decorations");
		decoParent = nullptr;
	}

	if (g_decorations.empty())
		return;

	decoParent = &g_scene->root->AddChild<scene::SceneNode>("decorations");

	for (size_t i = 0; i < g_decorations.size(); ++i)
	{
		auto& inst = g_decorations[i];
		auto* cached = GetCachedModel(inst.folder, inst.modelFile);
		if (!cached || !cached->mesh)
			continue;

		auto& node = decoParent->AddChild<scene::ModelNode>("deco_" + ModelKey(inst.folder, inst.modelFile));
		node.mesh = cached->mesh;
		node.material = cached->material;
		node.transform.position = inst.position;
		node.transform.rotation = glm::quat(glm::radians(inst.rotation));
		node.transform.scale    = inst.scale;
	}
}

//=============================================================================
void decorations::ClearCache()
{
	s_cache.clear();
	s_defaultMaterial.reset();
	s_previewMaterial.reset();
}
