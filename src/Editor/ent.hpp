#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

#include "grid.hpp"
#include "editor_math.hpp"

namespace ed
{
	class TexHandle;
	class ModelHandle;

	struct Ent
	{
		enum class DisplayMode : uint8_t
		{
			SPHERE,
			MODEL,
			SPRITE,
		};

		bool active = false;
		DisplayMode display = DisplayMode::SPHERE;
		glm::vec3 color = glm::vec3(1.0f);
		float radius = 0.0f;
		glm::vec3 lastRenderedPosition = glm::vec3(0.0f);
		int yaw = 0, pitch = 0;

		std::shared_ptr<ModelHandle> model;
		std::shared_ptr<TexHandle> texture;

		std::map<std::string, std::string> properties;

		Ent() = default;
		explicit Ent(float radius_);
	};

	void to_json(nlohmann::json& j, const Ent& ent);
	void from_json(const nlohmann::json& j, Ent& ent);

	constexpr float ENT_SPACING_DEFAULT = 2.0f;

	class EntGrid : public Grid<Ent>
	{
	public:
		EntGrid();
		explicit EntGrid(size_t width, size_t height, size_t length);

		void AddEnt(int i, int j, int k, Ent ent) { SetCel(i, j, k, ent); }
		void RemoveEnt(int i, int j, int k) { SetCel(i, j, k, Ent()); }
		bool HasEnt(int i, int j, int k) const { return GetCel(i, j, k).active; }
		Ent GetEnt(int i, int j, int k) const;
		void CopyEnts(int i, int j, int k, const EntGrid& src) { CopyCels(i, j, k, src); }
		EntGrid Subsection(int i, int j, int k, int w, int h, int l) const;

		std::vector<Ent> GetEntList() const;
	};
} // namespace ed