#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <memory>
#include <unordered_map>

//=============================================================================
// Forward declarations
//=============================================================================
namespace gr
{
	class Mesh;
	class Material;
} //namespace gr

namespace scene
{
	class SceneManager;
	class ModelNode;
} //namespace scene

//=============================================================================
namespace map
{
	//=== Texture IDs =========================================================
	enum FloorTex : uint8_t
	{
		FLOOR_NONE   = 0,
		FLOOR_GRASS  = 1,
		FLOOR_DIRT   = 2,
		FLOOR_STONE  = 3,
		FLOOR_SAND   = 4,
		FLOOR_WATER  = 5,
		FLOOR_WOOD   = 6,
		FLOOR_COUNT  = 7,

		FLOOR_FIRST  = 1,
	};

	enum WallTex : uint8_t
	{
		WALL_STONE   = 1,
		WALL_BRICK   = 2,
		WALL_DIRT    = 3,
		WALL_COUNT   = 4,

		WALL_FIRST   = 1,
	};

	enum TopTex : uint8_t
	{
		TOP_GRASS    = 1,
		TOP_STONE    = 2,
		TOP_DIRT     = 3,
		TOP_COUNT    = 4,

		TOP_FIRST    = 1,
	};

	//=== Tool IDs ============================================================
	enum class EditorTool : uint8_t
	{
		FloorBrush,
		MountainBrush,
		Eraser,
		Count,
	};

	//=== Slope data ==========================================================
	struct SideSlope final
	{
		int16_t blocks = 0; //< full tile widths
		int16_t pixels = 0; //< 0..16 (16 = 1 tile, in local 16ths)

		[[nodiscard]] float Total() const noexcept
		{
			return static_cast<float>(blocks) + static_cast<float>(pixels) / 16.0f;
		}
	};

	//=== Mountain data =======================================================
	struct MountainData final
	{
		bool    hasMountain   = false;
		int16_t heightBlocks = 1;
		int16_t heightPixels = 0; //< 0..16

		SideSlope slopeLeft;   //< X: protrusion on the -X side
		SideSlope slopeRight;  //< X: protrusion on the +X side
		SideSlope slopeFront;  //< Z: protrusion on the +Z side
		SideSlope slopeBack;   //< Z: protrusion on the -Z side

		uint8_t texWall = WALL_STONE; //< shared by all 4 walls
		uint8_t texTop  = TOP_GRASS;  //< top face texture
		uint8_t texBot  = WALL_STONE; //< bottom face texture

		[[nodiscard]] float Height() const noexcept
		{
			return static_cast<float>(heightBlocks) + static_cast<float>(heightPixels) / 16.0f;
		}
	};

	//=== Per-cell data =======================================================
	struct MapCell final
	{
		uint8_t      floorTex = FLOOR_NONE;
		MountainData mountain;
	};

	static constexpr int MAP_SIZE = 60;

	// Number of vertical bands per wall face to reduce texture skew on
	// sloped trapezoidal walls.  Higher = smoother but more geometry.
	static constexpr int WALL_SUBDIVISIONS = 8;

	//=== Material key for batching ===========================================
	struct MaterialKey final
	{
		uintptr_t texPtr   = 0; // address of the gpu::texture::TexturePtr (or 0 for color-only)
		uint32_t  texIndex = 0; //< texture array index or use pointer directly

		bool operator==(const MaterialKey& o) const noexcept = default;
	};

//=== Vertex/index batch ==================================================
struct MeshBatch final
{
	std::vector<gr::MeshVertex>         vertices;
	std::vector<uint32_t>               indices;
	std::shared_ptr<gr::Material>       material;
	std::string                         nodeName;
};

	//=== Editor state ========================================================
	struct EditorState final
	{
		EditorTool  activeTool       = EditorTool::FloorBrush;
		uint8_t     selectedFloorTex = FLOOR_GRASS;
		uint8_t     selectedWallTex  = WALL_STONE;
		uint8_t     selectedTopTex   = TOP_GRASS;

		int16_t     mountainHeightBlocks = 2;
		int16_t     mountainHeightPixels = 0;

		// Slope per side (blocks+pixels)
		int16_t     slopeBlocks = 0;
		int16_t     slopePixels = 8; // half-tile default slope

		bool        slopeLeft   = true;
		bool        slopeRight  = true;
		bool        slopeFront  = true;
		bool        slopeBack   = true;

		// Show current cell highlight
		int         hoverGridX  = -1;
		int         hoverGridZ  = -1;
	};

	//=== Map Editor ==========================================================
	class MapEditor final
	{
	public:
		MapEditor();
		~MapEditor();

		void Init();
		void Close();

		void Update(scene::SceneManager& scene);
		void RenderUI();

		// Tools
		void PaintCell(int gx, int gz);
		void EraseCell(int gx, int gz);

		// Call this when map data changes → rebuilds GPU geometry
		void RebuildGeometry(scene::SceneManager& scene);

		// Test map
		void CreateTestMap();

		// Access
		[[nodiscard]] const MapCell& GetCell(int gx, int gz) const;
		[[nodiscard]] MapCell&       GetCell(int gx, int gz);

		// Ray-pick: project screen coords onto y=0 plane
		[[nodiscard]] bool ScreenToGrid(int sx, int sy, int& outGx, int& outGz) const;

	private:
		void generateTextures();
		void buildFloorBatches(std::vector<MeshBatch>& batches);
		void buildMountainBatches(std::vector<MeshBatch>& batches);

		// Add a quad to a batch
		void addQuad(
			MeshBatch& batch,
			glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
			glm::vec3 n,
			glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3);

		// Add a wall face to a batch (trapezoid or rectangle).
		// Splits the wall into `subdivs` horizontal bands to minimise
		// texture skew on sloped walls.
		void addWallFace(
			MeshBatch& batch,
			glm::vec3 t0, glm::vec3 t1, // top edge
			glm::vec3 b0, glm::vec3 b1, // bottom edge
			glm::vec3 normal,
			float texRepeatV,
			int subdivs = WALL_SUBDIVISIONS);

		// Create a procedural texture with solid colour + noise
		[[nodiscard]] gpu::texture::TexturePtr makeProceduralTex(
			int w, int h,
			glm::vec3 baseColor,
			float noiseStrength,
			std::string_view name);

		// Create a procedural tiling pattern
		[[nodiscard]] gpu::texture::TexturePtr makeWallTex(
			int w, int h,
			glm::vec3 baseColor,
			glm::vec3 mortarColor,
			std::string_view name);

	private:
		std::array<std::array<MapCell, MAP_SIZE>, MAP_SIZE> m_grid;

		// Generated textures
		std::array<gpu::texture::TexturePtr, FLOOR_COUNT>    m_floorTex;
		std::array<gpu::texture::TexturePtr, WALL_COUNT>     m_wallTex;
		std::array<gpu::texture::TexturePtr, TOP_COUNT>      m_topTex;

		// Batched geometry nodes
		std::vector<std::unique_ptr<MeshBatch>> m_batches;

		// Sampler for all map textures
		gpu::texture::SamplerPtr m_sampler;

		EditorState m_state;

		bool m_dirty = true; //< rebuild geometry on next frame
	};

} //namespace map
