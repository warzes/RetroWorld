#include "stdafx.h"
#include "map_man.hpp"
#include "../assets.hpp"
#include "../text_util.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cassert>
#include <unordered_map>
//=============================================================================
namespace fs = std::filesystem;
//=============================================================================
ed::MapMan::MapMan()
	: _tileGrid(*this, 0, 0, 0)
	, _entGrid(0, 0, 0)
{}
//=============================================================================
ed::MapMan::~MapMan() = default;
//=============================================================================
void ed::MapMan::NewMap(int width, int height, int length)
{
	_tileGrid = TileGrid(*this,
		static_cast<size_t>(width),
		static_cast<size_t>(height),
		static_cast<size_t>(length));
	_entGrid = EntGrid(
		static_cast<size_t>(width),
		static_cast<size_t>(height),
		static_cast<size_t>(length));
	_undoHistory.clear();
	_redoHistory.clear();
	_numberOfChanges = 0;
	_willConvert = false;
}
//=============================================================================
void ed::MapMan::ExpandMap(Direction axis, int amount)
{
	if (amount <= 0) return;

	size_t newW = _tileGrid.GetWidth();
	size_t newH = _tileGrid.GetHeight();
	size_t newL = _tileGrid.GetLength();

	switch (axis)
	{
	case Direction::X_POS:
	case Direction::X_NEG: newW += static_cast<size_t>(amount); break;
	case Direction::Y_POS:
	case Direction::Y_NEG: newH += static_cast<size_t>(amount); break;
	case Direction::Z_POS:
	case Direction::Z_NEG: newL += static_cast<size_t>(amount); break;
	}

	TileGrid newTiles(*this, newW, newH, newL);
	EntGrid newEnts(newW, newH, newL);

	int dx = (axis == Direction::X_NEG) ? amount : 0;
	int dy = (axis == Direction::Y_NEG) ? amount : 0;
	int dz = (axis == Direction::Z_NEG) ? amount : 0;

	newTiles.CopyTiles(dx, dy, dz, _tileGrid);
	newEnts.CopyEnts(dx, dy, dz, _entGrid);

	_tileGrid = std::move(newTiles);
	_entGrid = std::move(newEnts);
	++_numberOfChanges;
}
//=============================================================================
void ed::MapMan::ShrinkMap()
{
	assert(_tileGrid.GetWidth() > 0 && _tileGrid.GetHeight() > 0 && _tileGrid.GetLength() > 0);

	// Find bounds of actual content
	size_t newW = _tileGrid.GetWidth();
	size_t newH = _tileGrid.GetHeight();
	size_t newL = _tileGrid.GetLength();

	// Shrink each axis by 1 (trim last row/column/layer)
	if (newW > 1) --newW;
	if (newH > 1) --newH;
	if (newL > 1) --newL;

	TileGrid newTiles(*this, newW, newH, newL);
	EntGrid newEnts(newW, newH, newL);

	newTiles.CopyTiles(0, 0, 0, _tileGrid);
	newEnts.CopyEnts(0, 0, 0, _entGrid);

	_tileGrid = std::move(newTiles);
	_entGrid = std::move(newEnts);
	++_numberOfChanges;
}
//=============================================================================
bool ed::MapMan::SaveTE3Map(const fs::path& filePath)
{
	nlohmann::json root;
	root["version"] = 3;
	root["width"] = static_cast<int>(_tileGrid.GetWidth());
	root["height"] = static_cast<int>(_tileGrid.GetHeight());
	root["length"] = static_cast<int>(_tileGrid.GetLength());
	root["spacing"] = _tileGrid.GetSpacing();
	root["tileData"] = _tileGrid.GetTileDataBase64();

	// Texture paths
	nlohmann::json texPaths = nlohmann::json::array();
	for (TexID i = 0; i < static_cast<TexID>(_texturePaths.size()); ++i)
		texPaths.push_back(_texturePaths[i].generic_string());
	root["texturePaths"] = texPaths;

	// Model paths
	nlohmann::json modPaths = nlohmann::json::array();
	for (ModelID i = 0; i < static_cast<ModelID>(_modelPaths.size()); ++i)
		modPaths.push_back(_modelPaths[i].generic_string());
	root["modelPaths"] = modPaths;

	// Entities
	{
		nlohmann::json ents = nlohmann::json::object();
		nlohmann::json byPos = nlohmann::json::object();
		for (size_t i = 0; i < _entGrid.GetWidth() * _entGrid.GetHeight() * _entGrid.GetLength(); ++i)
		{
			if (_entGrid.GetEntList().empty()) continue;
			// For now skip per-cell entity serialization in new format
		}
		// Write ent list as array
		auto entList = _entGrid.GetEntList();
		nlohmann::json entArray = nlohmann::json::array();
		for (const auto& ent : entList)
		{
			nlohmann::json ej;
			to_json(ej, ent);
			entArray.push_back(ej);
		}
		root["entities"] = entArray;
	}

	// Default camera position/angles
	if (_defaultCameraPosition != glm::vec3(0.0f) || _defaultCameraAngles != glm::vec3(0.0f))
	{
		root["defaultCameraPosition"] = nlohmann::json::array({
			_defaultCameraPosition.x, _defaultCameraPosition.y, _defaultCameraPosition.z });
		root["defaultCameraAngles"] = nlohmann::json::array({
			_defaultCameraAngles.x, _defaultCameraAngles.y, _defaultCameraAngles.z });
	}

	std::ofstream fout(filePath);
	if (!fout.is_open())
	{
		core::Error("MapMan: Failed to write '" + filePath.generic_string() + "'");
		return false;
	}
	fout << root.dump(1, '\t') << std::endl;
	_numberOfChanges = 0;
	return true;
}
//=============================================================================
	bool ed::MapMan::LoadTE3Map(const fs::path& filePath)
	{
		std::ifstream fin(filePath);
		if (!fin.is_open())
		{
			core::Error("MapMan: Failed to open '" + filePath.generic_string() + "'");
			return false;
	}

	nlohmann::json root;
	try { fin >> root; }
	catch (const std::exception& e)
	{
		core::Error("MapMan: JSON parse error: " + std::string(e.what()));
		return false;
	}

	int version = root.value("version", 2);
	int width = root["width"].get<int>();
	int height = root["height"].get<int>();
	int length = root["length"].get<int>();
	float spacing = root.value("spacing", TILE_SPACING_DEFAULT);

	NewMap(width, height, length);
	_tileGrid = TileGrid(*this, static_cast<size_t>(width), static_cast<size_t>(height), static_cast<size_t>(length), spacing, Tile());

	// Tile data
	if (root.contains("tileData"))
	{
		if (version >= 3)
			_tileGrid.SetTileDataBase64(root["tileData"].get<std::string>());
		else
			_tileGrid.SetTileDataBase64OldFormat(root["tileData"].get<std::string>());
	}

	// Texture/model paths
	if (root.contains("texturePaths"))
	{
		for (const auto& p : root["texturePaths"])
		{
			fs::path path = p.get<std::string>();
			_texturePaths.push_back(path);
			auto handle = Assets::GetTexture(path);
			_textureList.push_back(handle);
		}
	}

	if (root.contains("modelPaths"))
	{
		for (const auto& p : root["modelPaths"])
		{
			fs::path path = p.get<std::string>();
			_modelPaths.push_back(path);
			auto handle = Assets::GetModel(path);
			_modelList.push_back(handle);
		}
	}

	// Entities
	if (root.contains("entities"))
	{
		for (const auto& ej : root["entities"])
		{
			Ent ent;
			from_json(ej, ent);
			// Place at position from JSON
			glm::vec3 pos = ent.lastRenderedPosition;
			auto gp = _entGrid.WorldToGridPos(pos);
			int gi = static_cast<int>(gp.x);
			int gj = static_cast<int>(gp.y);
			int gk = static_cast<int>(gp.z);
			_entGrid.AddEnt(gi, gj, gk, ent);
		}
	}

	// Default camera
	if (root.contains("defaultCameraPosition"))
	{
		auto& arr = root["defaultCameraPosition"];
		_defaultCameraPosition = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
	}
	if (root.contains("defaultCameraAngles"))
	{
		auto& arr = root["defaultCameraAngles"];
		_defaultCameraAngles = glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
	}

	_numberOfChanges = 0;
	_willConvert = (version < 3);
	return true;
}
//=============================================================================
	bool ed::MapMan::LoadTE2Map(const fs::path& filePath)
	{
		std::ifstream fin(filePath);
		if (!fin.is_open())
		{
			core::Error("MapMan: Failed to open '" + filePath.generic_string() + "'");
			return false;
	}

	nlohmann::json root;
	try { fin >> root; }
	catch (const std::exception& e)
	{
		core::Error("MapMan: JSON parse error: " + std::string(e.what()));
		return false;
	}

	int width = root["width"].get<int>();
	int height = root["height"].get<int>();
	int length = root["length"].get<int>();

	NewMap(width, height, length);
	_tileGrid = TileGrid(*this, static_cast<size_t>(width), static_cast<size_t>(height), static_cast<size_t>(length));

	if (root.contains("tileData"))
		_tileGrid.SetTileDataBase64OldFormat(root["tileData"].get<std::string>());

	if (root.contains("texturePaths"))
	{
		for (const auto& p : root["texturePaths"])
		{
			_texturePaths.push_back(p.get<std::string>());
			_textureList.push_back(Assets::GetTexture(_texturePaths.back()));
		}
	}

	_numberOfChanges = 0;
	_willConvert = true;
	return true;
}
//=============================================================================
void ed::MapMan::execute(std::shared_ptr<Action> action)
{
	_sceneGraphDirty = true;
	action->Do(*this);
	_undoHistory.push_back(std::move(action));
	_redoHistory.clear();
	++_numberOfChanges;
}
//=============================================================================
void ed::MapMan::Undo()
{
	if (_undoHistory.empty()) return;
	_sceneGraphDirty = true;
	auto action = std::move(_undoHistory.back());
	_undoHistory.pop_back();
	action->Undo(*this);
	_redoHistory.push_front(std::move(action));
	++_numberOfChanges;
}
//=============================================================================
void ed::MapMan::Redo()
{
	if (_redoHistory.empty()) return;
	_sceneGraphDirty = true;
	auto action = std::move(_redoHistory.front());
	_redoHistory.pop_front();
	action->Do(*this);
	_undoHistory.push_back(std::move(action));
	++_numberOfChanges;
}
//=============================================================================
void ed::MapMan::ExecuteTileAction(size_t i, size_t j, size_t k, size_t w, size_t h, size_t l, Tile newTile)
{
	auto prevState = _tileGrid.Subsection(
		static_cast<int>(i), static_cast<int>(j), static_cast<int>(k),
		static_cast<int>(w), static_cast<int>(h), static_cast<int>(l));

	_tileGrid.SetTileRect(
		static_cast<int>(i), static_cast<int>(j), static_cast<int>(k),
		static_cast<int>(w), static_cast<int>(h), static_cast<int>(l), newTile);

	auto newState = _tileGrid.Subsection(
		static_cast<int>(i), static_cast<int>(j), static_cast<int>(k),
		static_cast<int>(w), static_cast<int>(h), static_cast<int>(l));

	execute(std::make_shared<TileAction>(i, j, k, std::move(prevState), std::move(newState)));
}
//=============================================================================
void ed::MapMan::ExecuteTileAction(size_t i, size_t j, size_t k, size_t w, size_t h, size_t l, TileGrid brush)
{
	auto prevState = _tileGrid.Subsection(
		static_cast<int>(i), static_cast<int>(j), static_cast<int>(k),
		static_cast<int>(w), static_cast<int>(h), static_cast<int>(l));

	_tileGrid.CopyTiles(static_cast<int>(i), static_cast<int>(j), static_cast<int>(k), brush, true);

	auto newState = _tileGrid.Subsection(
		static_cast<int>(i), static_cast<int>(j), static_cast<int>(k),
		static_cast<int>(w), static_cast<int>(h), static_cast<int>(l));

	execute(std::make_shared<TileAction>(i, j, k, std::move(prevState), std::move(newState)));
}
//=============================================================================
void ed::MapMan::ExecuteEntPlacement(int i, int j, int k, Ent newEnt)
{
	bool overwrite = _entGrid.HasEnt(i, j, k);
	Ent oldEnt = overwrite ? _entGrid.GetEnt(i, j, k) : Ent();
	newEnt.lastRenderedPosition = _entGrid.GridToWorldPos(glm::vec3(
		static_cast<float>(i), static_cast<float>(j), static_cast<float>(k)), true);
	_entGrid.AddEnt(i, j, k, newEnt);
	execute(std::make_shared<EntAction>(
		static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k),
		overwrite, false, oldEnt, newEnt));
}
//=============================================================================
void ed::MapMan::ExecuteEntRemoval(int i, int j, int k)
{
	if (!_entGrid.HasEnt(i, j, k)) return;
	Ent oldEnt = _entGrid.GetEnt(i, j, k);
	_entGrid.RemoveEnt(i, j, k);
	execute(std::make_shared<EntAction>(
		static_cast<size_t>(i), static_cast<size_t>(j), static_cast<size_t>(k),
		true, true, oldEnt, Ent()));
}
//=============================================================================
ed::TexID ed::MapMan::GetOrAddTexID(const fs::path& texturePath)
{
	for (TexID i = 0; i < static_cast<TexID>(_texturePaths.size()); ++i)
	{
		if (_texturePaths[i] == texturePath)
			return i;
	}
	TexID id = static_cast<TexID>(_texturePaths.size());
	_texturePaths.push_back(texturePath);
	_textureList.push_back(Assets::GetTexture(texturePath));
	return id;
}
//=============================================================================
ed::ModelID ed::MapMan::GetOrAddModelID(const fs::path& modelPath)
{
	for (ModelID i = 0; i < static_cast<ModelID>(_modelPaths.size()); ++i)
	{
		if (_modelPaths[i] == modelPath)
			return i;
	}
	ModelID id = static_cast<ModelID>(_modelPaths.size());
	_modelPaths.push_back(modelPath);
	_modelList.push_back(Assets::GetModel(modelPath));
	return id;
}
//=============================================================================
fs::path ed::MapMan::PathFromTexID(TexID id) const
{
	if (id >= 0 && id < static_cast<TexID>(_texturePaths.size()))
		return _texturePaths[id];
	return {};
}
//=============================================================================
fs::path ed::MapMan::PathFromModelID(ModelID id) const
{
	if (id >= 0 && id < static_cast<ModelID>(_modelPaths.size()))
		return _modelPaths[id];
	return {};
}
//=============================================================================
namespace
{
	// Build a flat grid mesh composed of thin quads (each grid line is a thin rect)
	std::shared_ptr<gr::Mesh> CreateGridMesh(float width, float length, int divX, int divZ, float lineWidth = 0.04f)
	{
		float halfLW = lineWidth * 0.5f;
		std::vector<gr::MeshVertex> vertices;
		std::vector<uint32_t> indices;
		uint32_t idx = 0;

		float spacingX = divX > 0 ? width / static_cast<float>(divX) : width;
		float spacingZ = divZ > 0 ? length / static_cast<float>(divZ) : length;

		// Lines along X
		for (int z = 0; z <= divZ; ++z)
		{
			float zPos = z * spacingZ;
			vertices.push_back({ .position = {0,           0, zPos - halfLW}, .normal = {0, 1, 0}, .uv = {0, 0} });
			vertices.push_back({ .position = {width,       0, zPos - halfLW}, .normal = {0, 1, 0}, .uv = {1, 0} });
			vertices.push_back({ .position = {width,       0, zPos + halfLW}, .normal = {0, 1, 0}, .uv = {1, 1} });
			vertices.push_back({ .position = {0,           0, zPos + halfLW}, .normal = {0, 1, 0}, .uv = {0, 1} });
			indices.push_back(idx); indices.push_back(idx + 1); indices.push_back(idx + 2);
			indices.push_back(idx); indices.push_back(idx + 2); indices.push_back(idx + 3);
			idx += 4;
		}

		// Lines along Z
		for (int x = 0; x <= divX; ++x)
		{
			float xPos = x * spacingX;
			vertices.push_back({ .position = {xPos - halfLW, 0, 0},     .normal = {0, 1, 0}, .uv = {0, 0} });
			vertices.push_back({ .position = {xPos + halfLW, 0, 0},     .normal = {0, 1, 0}, .uv = {1, 0} });
			vertices.push_back({ .position = {xPos + halfLW, 0, length}, .normal = {0, 1, 0}, .uv = {1, 1} });
			vertices.push_back({ .position = {xPos - halfLW, 0, length}, .normal = {0, 1, 0}, .uv = {0, 1} });
			indices.push_back(idx); indices.push_back(idx + 1); indices.push_back(idx + 2);
			indices.push_back(idx); indices.push_back(idx + 2); indices.push_back(idx + 3);
			idx += 4;
		}

		auto mesh = std::make_shared<gr::Mesh>();
		mesh->vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);
		mesh->vbo = gpu::buffer::CreateBuffer(vertices.data(), vertices.size() * sizeof(gr::MeshVertex));
		mesh->ibo = gpu::buffer::CreateBuffer(indices.data(), indices.size() * sizeof(uint32_t));
		mesh->vertexCount = static_cast<uint32_t>(vertices.size());
		mesh->indexCount = static_cast<uint32_t>(indices.size());
		mesh->isIndexed = true;

		std::vector<glm::vec3> positions(vertices.size());
		for (size_t i = 0; i < vertices.size(); ++i)
			positions[i] = vertices[i].position;
		mesh->ComputeAABB(positions);
		return mesh;
	}
}
//=============================================================================
void ed::MapMan::SyncSceneGraph(scene::SceneNode& root, scene::SceneManager& sm)
{
	_sceneGraphDirty = false;

	// Remove previous editor-generated nodes
	root.RemoveChild("tilemap");
	root.RemoveChild("grid");
	root.RemoveChild("entities");

	float spacing = _tileGrid.GetSpacing();
	size_t w = _tileGrid.GetWidth();
	size_t h = _tileGrid.GetHeight();
	size_t l = _tileGrid.GetLength();

	// ---- Tile ChunkNode ----
	auto& chunkNode = root.AddChild<scene::ChunkNode>("tilemap");

	// Group tiles by (mesh, texture)
	struct BatchKey
	{
		std::shared_ptr<gr::Mesh> mesh;
		gpu::texture::TexturePtr texture;
	};
	struct BatchKeyHash
	{
		size_t operator()(const BatchKey& k) const noexcept
		{
			return std::hash<std::shared_ptr<gr::Mesh>>()(k.mesh)
				^ (reinterpret_cast<size_t>(k.texture.get()) << 1);
		}
	};
	struct BatchKeyEq
	{
		bool operator()(const BatchKey& a, const BatchKey& b) const noexcept
		{
			return a.mesh == b.mesh && a.texture == b.texture;
		}
	};

	std::unordered_map<BatchKey, std::vector<glm::mat4>, BatchKeyHash, BatchKeyEq> batchGroups;

	for (size_t y = 0; y < h; ++y)
	{
		for (size_t z = 0; z < l; ++z)
		{
			for (size_t x = 0; x < w; ++x)
			{
				Tile tile = _tileGrid.GetTile(
					static_cast<int>(x), static_cast<int>(y), static_cast<int>(z));
				if (!tile) continue;
				if (tile.shape < 0 || tile.shape >= static_cast<ModelID>(_modelList.size())) continue;
				if (tile.textures[0] < 0 || tile.textures[0] >= static_cast<TexID>(_textureList.size())) continue;

				auto meshHandle = _modelList[tile.shape];
				if (!meshHandle) continue;

				auto texHandle = _textureList[tile.textures[0]];
				if (!texHandle) continue;

				glm::vec3 worldPos = _tileGrid.GridToWorldPos(
					glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)), true);
				glm::mat4 transform = glm::translate(glm::mat4(1.0f), worldPos);
				transform = transform * TileRotationMatrix(tile.yaw, tile.pitch);

				// Primary submesh with primary texture
				auto primaryMesh = meshHandle->GetMesh(0);
				auto primaryTex = texHandle->GetTexture();
				if (primaryMesh && primaryTex)
				{
					BatchKey key{ primaryMesh, primaryTex };
					batchGroups[key].push_back(transform);
				}

				// Secondary submesh with secondary texture (if available)
				if (meshHandle->GetMeshCount() > 1
					&& tile.textures[1] >= 0
					&& tile.textures[1] < static_cast<TexID>(_textureList.size()))
				{
					auto secondaryMesh = meshHandle->GetMesh(1);
					auto secondaryTexHandle = _textureList[tile.textures[1]];
					if (secondaryMesh && secondaryTexHandle)
					{
						auto secondaryTex = secondaryTexHandle->GetTexture();
						if (secondaryTex)
						{
							BatchKey key{ secondaryMesh, secondaryTex };
							batchGroups[key].push_back(transform);
						}
					}
				}
			}
		}
	}

	for (auto& [key, transforms] : batchGroups)
	{
		auto material = std::make_shared<gr::Material>();
		material->albedoMap = key.texture;
		material->albedoColor = glm::vec3(1.0f);
		material->ambientColor = glm::vec3(0.3f);
		material->specularColor = glm::vec3(0.5f);
		material->shininess = 32.0f;
		material->cullMode = gpu::CullMode::Back;

		chunkNode.AddBatch(key.mesh, material, std::move(transforms));
	}
	chunkNode.RebuildAABB();

	// ---- Grid ModelNode ----
	{
		float gridW = static_cast<float>(w) * spacing;
		float gridL = static_cast<float>(l) * spacing;
		auto gridMesh = CreateGridMesh(gridW, gridL, static_cast<int>(w), static_cast<int>(l));
		auto gridMat = std::make_shared<gr::Material>();
		gridMat->albedoColor = glm::vec3(0.3f, 0.3f, 0.35f);
		gridMat->ambientColor = glm::vec3(1.0f);
		gridMat->specularColor = glm::vec3(0.0f);
		gridMat->shininess = 1.0f;
		gridMat->cullMode = gpu::CullMode::None;
		gridMat->castShadow = false;
		gridMat->receiveShadow = false;

		auto& gridNode = root.AddChild<scene::ModelNode>("grid");
		gridNode.mesh = std::move(gridMesh);
		gridNode.material = std::move(gridMat);
		gridNode.castShadow = false;
		gridNode.receiveShadow = false;
	}

	// ---- Entity ModelNodes ----
	{
		auto& entRoot = root.AddChild<scene::SceneNode>("entities");

		const auto& entList = _entGrid.GetEntList();
		static auto s_sharedSphere = std::make_shared<gr::Mesh>(gr::Mesh::CreateSphere(8, 12));
		for (size_t ei = 0; ei < entList.size(); ++ei)
		{
			const auto& ent = entList[ei];
			if (!ent.active) continue;

			glm::vec3 pos = ent.lastRenderedPosition;

			if (ent.display == Ent::DisplayMode::SPHERE)
			{
				auto& node = entRoot.AddChild<scene::ModelNode>("ent_sph");
				node.mesh = s_sharedSphere;
				node.material = std::make_shared<gr::Material>();
				node.material->albedoColor = ent.color;
				node.material->ambientColor = glm::vec3(0.3f);
				node.material->specularColor = glm::vec3(0.3f);
				node.material->shininess = 16.0f;
				node.material->castShadow = false;
				node.material->receiveShadow = false;
				node.transform.position = pos;
				float r = (ent.radius > 0.0f) ? ent.radius : 1.0f;
				node.transform.scale = glm::vec3(r);
			}
			else if (ent.display == Ent::DisplayMode::MODEL)
			{
				auto mesh = ent.model ? ent.model->GetMesh() : nullptr;
				if (!mesh) continue;

				auto& node = entRoot.AddChild<scene::ModelNode>("ent_mdl");
				node.mesh = mesh;
				node.material = std::make_shared<gr::Material>();
				if (ent.texture)
				{
					node.material->albedoColor = glm::vec3(1.0f);
					node.material->albedoMap = ent.texture->GetTexture();
				}
				else
				{
					node.material->albedoColor = ent.color;
				}
				node.material->ambientColor = glm::vec3(0.3f);
				node.material->specularColor = glm::vec3(0.3f);
				node.material->shininess = 16.0f;
				node.material->castShadow = false;
				node.material->receiveShadow = false;
				node.transform.position = pos;
			}
			else if (ent.display == Ent::DisplayMode::SPRITE)
			{
				auto& node = entRoot.AddChild<scene::ModelNode>("ent_spr");
				node.mesh = std::make_shared<gr::Mesh>(gr::Mesh::CreateQuad());
				node.material = std::make_shared<gr::Material>();
				if (ent.texture)
				{
					node.material->albedoMap = ent.texture->GetTexture();
					node.material->albedoColor = glm::vec3(1.0f);
				}
				else
				{
					node.material->albedoColor = ent.color;
				}
				node.material->ambientColor = glm::vec3(1.0f);
				node.material->castShadow = false;
				node.material->receiveShadow = false;
				node.transform.position = pos;
				float s = ent.radius > 0.0f ? ent.radius : 1.0f;
				node.transform.scale = glm::vec3(s);
			}
		}
	}
}
//=============================================================================
void ed::MapMan::ClearSceneGraph()
{
	// Scene graph clearing is handled externally by the App
}
//=============================================================================