#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <gr_mesh.h>
#include <gr_material.h>
#include <sc_modelNode.h>

namespace decorations
{
	struct Instance final
	{
		std::string folder;
		std::string modelFile;
		glm::vec3   position;
		glm::vec3   rotation; // euler angles (degrees)
		glm::vec3   scale    { 1.0f };
	};

	struct CachedModel final
	{
		std::shared_ptr<gr::Mesh>     mesh;
		std::shared_ptr<gr::Material> material;
		math::AABB                    aabb;
	};

	std::vector<std::string> ScanFolders();
	std::vector<std::string> ScanModels(const std::string& folder);
	bool EnsureModelLoaded(const std::string& folder, const std::string& modelFile);
	CachedModel* GetCachedModel(const std::string& folder, const std::string& modelFile);
	std::string ModelKey(const std::string& folder, const std::string& modelFile);

	// Scene node management
	scene::ModelNode* CreateSceneNode(const std::string& folder, const std::string& modelFile, const glm::vec3& pos);
	void DestroySceneNode(int index);
	void UpdateSceneTransform(int index);
	void RebuildAllSceneNodes();

	// Default material for preview / no-texture models
	std::shared_ptr<gr::Material> GetDefaultMaterial();
	std::shared_ptr<gr::Material> GetPreviewMaterial();

	void ClearCache();
} // namespace decorations
