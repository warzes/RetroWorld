#include "stdafx.h"
#include "MapEditor.h"
#include <random>

//=============================================================================
namespace map
{
namespace
{
	// Grid highlight colour (cyan outline)
	constexpr glm::vec3 HIGHLIGHT_COLOR = { 0.0f, 1.0f, 1.0f };

	// Simple hash-based noise for procedural textures
	[[nodiscard]] float hash(float x, float y) noexcept
	{
		float n = sinf(x * 127.1f + y * 311.7f) * 43758.5453f;
		return n - floorf(n);
	}

	[[nodiscard]] float lerp(float a, float b, float t) noexcept
	{
		return a + (b - a) * t;
	}

	[[nodiscard]] float smoothNoise(float x, float y) noexcept
	{
		int ix = static_cast<int>(floorf(x));
		int iy = static_cast<int>(floorf(y));
		float fx = x - floorf(x);
		float fy = y - floorf(y);
		fx = fx * fx * (3.0f - 2.0f * fx);
		fy = fy * fy * (3.0f - 2.0f * fy);

		float a = hash(static_cast<float>(ix), static_cast<float>(iy));
		float b = hash(static_cast<float>(ix + 1), static_cast<float>(iy));
		float c = hash(static_cast<float>(ix), static_cast<float>(iy + 1));
		float d = hash(static_cast<float>(ix + 1), static_cast<float>(iy + 1));
		return lerp(lerp(a, b, fx), lerp(c, d, fx), fy);
	}

	[[nodiscard]] float fbm(float x, float y, int octaves = 3) noexcept
	{
		float value = 0.0f;
		float amp = 0.5f;
		float freq = 1.0f;
		for (int i = 0; i < octaves; ++i)
		{
			value += amp * smoothNoise(x * freq, y * freq);
			freq *= 2.0f;
			amp *= 0.5f;
		}
		return value;
	}

	// Clamp byte
	[[nodiscard]] uint8_t c(float v) noexcept
	{
		return static_cast<uint8_t>(std::clamp(v, 0.0f, 255.0f));
	}

} // anonymous namespace

//=============================================================================
MapEditor::MapEditor() = default;
MapEditor::~MapEditor() = default;

//=============================================================================
void MapEditor::Init()
{
	// Create sampler for tiled textures
	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Linear;
	ss.magFilter = gpu::Filter::Linear;
	ss.mipmapFilter = gpu::Filter::None;
	ss.addressModeU = gpu::AddressMode::Repeat;
	ss.addressModeV = gpu::AddressMode::Repeat;
	ss.addressModeW = gpu::AddressMode::Repeat;
	m_sampler = gpu::texture::CreateSampler(ss);

	generateTextures();

	CreateTestMap();
}

//=============================================================================
void MapEditor::generateTextures()
{
	constexpr int TEX_SIZE = 64;

	//=== Floor textures ======================================================
	auto makeFloor = [&](glm::vec3 base, float noise, std::string_view name) -> gpu::texture::TexturePtr
		{
			return makeProceduralTex(TEX_SIZE, TEX_SIZE, base, noise, name);
		};

	m_floorTex[FLOOR_GRASS] = makeFloor({ 0.2f, 0.7f, 0.15f }, 0.25f, "floor_grass");
	m_floorTex[FLOOR_DIRT]  = makeFloor({ 0.55f, 0.35f, 0.15f }, 0.20f, "floor_dirt");
	m_floorTex[FLOOR_STONE] = makeFloor({ 0.45f, 0.45f, 0.45f }, 0.30f, "floor_stone");
	m_floorTex[FLOOR_SAND]  = makeFloor({ 0.76f, 0.70f, 0.50f }, 0.15f, "floor_sand");
	m_floorTex[FLOOR_WATER] = makeFloor({ 0.10f, 0.30f, 0.70f }, 0.10f, "floor_water");
	m_floorTex[FLOOR_WOOD]  = makeFloor({ 0.50f, 0.30f, 0.10f }, 0.10f, "floor_wood");

	//=== Wall textures (brick/stone pattern) =================================
	m_wallTex[WALL_STONE] = makeWallTex(TEX_SIZE, TEX_SIZE,
		{ 0.50f, 0.50f, 0.50f }, { 0.25f, 0.25f, 0.25f }, "wall_stone");
	m_wallTex[WALL_BRICK] = makeWallTex(TEX_SIZE, TEX_SIZE,
		{ 0.70f, 0.25f, 0.15f }, { 0.35f, 0.20f, 0.12f }, "wall_brick");
	m_wallTex[WALL_DIRT]  = makeWallTex(TEX_SIZE, TEX_SIZE,
		{ 0.55f, 0.38f, 0.18f }, { 0.35f, 0.25f, 0.12f }, "wall_dirt");

	//=== Top textures ========================================================
	m_topTex[TOP_GRASS] = makeFloor({ 0.15f, 0.65f, 0.10f }, 0.30f, "top_grass");
	m_topTex[TOP_STONE] = makeFloor({ 0.40f, 0.40f, 0.42f }, 0.25f, "top_stone");
	m_topTex[TOP_DIRT]  = makeFloor({ 0.50f, 0.32f, 0.14f }, 0.25f, "top_dirt");
}

//=============================================================================
gpu::texture::TexturePtr MapEditor::makeProceduralTex(
	int w, int h,
	glm::vec3 baseColor,
	float noiseStrength,
	std::string_view name)
{
	std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			float fx = static_cast<float>(x) / static_cast<float>(w) * 4.0f;
			float fy = static_cast<float>(y) / static_cast<float>(h) * 4.0f;
			float n = fbm(fx, fy, 3);
			float variation = (n - 0.5f) * 2.0f * noiseStrength;

			size_t idx = static_cast<size_t>(y) * w * 4 + static_cast<size_t>(x) * 4;
			pixels[idx + 0] = c((baseColor.x + variation) * 255.0f);
			pixels[idx + 1] = c((baseColor.y + variation) * 255.0f);
			pixels[idx + 2] = c((baseColor.z + variation) * 255.0f);
			pixels[idx + 3] = 255;
		}
	}

	auto tex = gpu::texture::CreateTexture2D(
		{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) },
		gpu::Format::R8G8B8A8_UNORM,
		name);

	gpu::texture::UpdateImage(tex, {
		.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1},
		.format = gpu::UploadFormat::RGBA,
		.type   = gpu::UploadType::UBYTE,
		.pixels = pixels.data(),
		});

	return tex;
}

//=============================================================================
gpu::texture::TexturePtr MapEditor::makeWallTex(
	int w, int h,
	glm::vec3 baseColor,
	glm::vec3 mortarColor,
	std::string_view name)
{
	std::vector<uint8_t> pixels(static_cast<size_t>(w) * h * 4);

	// Brick-style pattern
	int brickW = w / 4;
	int brickH = h / 4;

	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			int row = y / brickH;
			int col = x / brickW;

			// Offset every other row by half a brick
			bool offset = (row & 1) != 0;
			float px = static_cast<float>(x) + (offset ? brickW / 2.0f : 0.0f);
			float py = static_cast<float>(y);

			bool isMortar = false;
			if (row & 1)
			{
				// Odd rows: mortar at the gap between offset bricks
				float localX = fmodf(px, static_cast<float>(brickW));
				isMortar = localX < 2.0f || localX > static_cast<float>(brickW) - 2.0f;
			}
			else
			{
				float localX = fmodf(px, static_cast<float>(brickW));
				isMortar = localX < 2.0f || localX > static_cast<float>(brickW) - 2.0f;
			}

			float localY = fmodf(py, static_cast<float>(brickH));
			isMortar = isMortar || localY < 2.0f || localY > static_cast<float>(brickH) - 2.0f;

			// Noise on the brick face
			float fx = static_cast<float>(x) / static_cast<float>(w) * 8.0f;
			float fy = static_cast<float>(y) / static_cast<float>(h) * 8.0f;
			float n = fbm(fx, fy, 2) * 0.15f;

			glm::vec3 color = isMortar ? mortarColor : baseColor;
			color.x += n;
			color.y += n;
			color.z += n;

			size_t idx = static_cast<size_t>(y) * w * 4 + static_cast<size_t>(x) * 4;
			pixels[idx + 0] = c(color.x * 255.0f);
			pixels[idx + 1] = c(color.y * 255.0f);
			pixels[idx + 2] = c(color.z * 255.0f);
			pixels[idx + 3] = 255;
		}
	}

	auto tex = gpu::texture::CreateTexture2D(
		{ static_cast<uint32_t>(w), static_cast<uint32_t>(h) },
		gpu::Format::R8G8B8A8_UNORM,
		name);

	gpu::texture::UpdateImage(tex, {
		.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1},
		.format = gpu::UploadFormat::RGBA,
		.type   = gpu::UploadType::UBYTE,
		.pixels = pixels.data(),
		});

	return tex;
}

//=============================================================================
void MapEditor::Close()
{
	m_batches.clear();
	m_floorTex.fill(nullptr);
	m_wallTex.fill(nullptr);
	m_topTex.fill(nullptr);
	m_sampler.reset();
}

//=============================================================================
void MapEditor::CreateTestMap()
{
	// Clear map first
	for (auto& row : m_grid)
		for (auto& cell : row)
			cell = MapCell{};

	//=== Floor: 60x60 with various textures ==================================
	for (int z = 0; z < MAP_SIZE; ++z)
	{
		for (int x = 0; x < MAP_SIZE; ++x)
		{
			// Grass base
			uint8_t tex = FLOOR_GRASS;

			// Dirt patches
			if ((x / 10 + z / 10) % 3 == 0 && (x % 5 < 3) && (z % 5 < 3))
				tex = FLOOR_DIRT;

			// Stone areas
			if (x >= 20 && x < 25 && z >= 20 && z < 25)
				tex = FLOOR_STONE;

			// Sand near one edge
			if (z >= 50 && z < 60 && x >= 40 && x < 60)
				tex = FLOOR_SAND;

			// Water pool
			if (x >= 10 && x < 15 && z >= 10 && z < 15)
				tex = FLOOR_WATER;

			// Wood platform
			if (x >= 30 && x < 35 && z >= 30 && z < 35)
				tex = FLOOR_WOOD;

			m_grid[z][x].floorTex = tex;
		}
	}

	//=== Mountains ===========================================================

	// Simple block mountains (no slope)
	auto addMountain = [&](int gx, int gz, int heightBlocks, int heightPixels,
		uint8_t wallTex, uint8_t topTex)
		{
			auto& cell = m_grid[gz][gx];
			auto& ml = cell.mountainStack.emplace_back();
			ml.hasMountain = true;
			ml.heightBlocks = static_cast<int16_t>(heightBlocks);
			ml.heightPixels = static_cast<int16_t>(heightPixels);
			ml.texWall = wallTex;
			ml.texTop = topTex;
		};

	// Mountain with slope
	auto addSlopeMountain = [&](int gx, int gz,
		int heightBlocks, int heightPixels,
		int slopeBlocks, int slopePixels,
		bool slopeL, bool slopeR, bool slopeF, bool slopeB,
		uint8_t wallTex, uint8_t topTex)
		{
			auto& cell = m_grid[gz][gx];
			auto& ml = cell.mountainStack.emplace_back();
			ml.hasMountain = true;
			ml.heightBlocks = static_cast<int16_t>(heightBlocks);
			ml.heightPixels = static_cast<int16_t>(heightPixels);
			ml.texWall = wallTex;
			ml.texTop = topTex;
			if (slopeL) { ml.slopeLeft.blocks = static_cast<int16_t>(slopeBlocks);
				ml.slopeLeft.pixels = static_cast<int16_t>(slopePixels); }
			if (slopeR) { ml.slopeRight.blocks = static_cast<int16_t>(slopeBlocks);
				ml.slopeRight.pixels = static_cast<int16_t>(slopePixels); }
			if (slopeF) { ml.slopeFront.blocks = static_cast<int16_t>(slopeBlocks);
				ml.slopeFront.pixels = static_cast<int16_t>(slopePixels); }
			if (slopeB) { ml.slopeBack.blocks = static_cast<int16_t>(slopeBlocks);
				ml.slopeBack.pixels = static_cast<int16_t>(slopePixels); }
		};

	// Block mountains cluster
	addMountain(5, 5, 1, 0, WALL_STONE, TOP_GRASS);
	addMountain(6, 5, 1, 0, WALL_STONE, TOP_GRASS);
	addMountain(5, 6, 1, 0, WALL_STONE, TOP_GRASS);
	addMountain(6, 6, 1, 8, WALL_STONE, TOP_GRASS);

	// Tall block
	addMountain(8, 5, 3, 0, WALL_BRICK, TOP_DIRT);
	addMountain(8, 6, 3, 0, WALL_BRICK, TOP_DIRT);

	// Slope mountains
	addSlopeMountain(15, 15, 2, 0, 0, 8, true, true, true, true, WALL_STONE, TOP_GRASS);
	addSlopeMountain(18, 15, 2, 0, 0, 12, true, true, true, true, WALL_DIRT, TOP_DIRT);
	addSlopeMountain(15, 18, 1, 0, 0, 6, true, true, true, true, WALL_BRICK, TOP_STONE);

	// Slope on only one side
	addSlopeMountain(22, 10, 2, 0, 0, 10, true, false, false, false, WALL_STONE, TOP_GRASS);

	// Slope on two adjacent sides
	addSlopeMountain(22, 14, 2, 0, 0, 8, true, false, true, false, WALL_DIRT, TOP_DIRT);

	// Wall at different height + pixel
	addMountain(25, 5, 2, 8, WALL_BRICK, TOP_STONE);

	// Another cluster
	addSlopeMountain(40, 40, 3, 0, 0, 6, true, true, true, true, WALL_STONE, TOP_GRASS);
	addSlopeMountain(42, 40, 2, 0, 0, 4, true, true, true, true, WALL_DIRT, TOP_DIRT);
	addSlopeMountain(40, 42, 1, 8, 0, 3, true, true, true, true, WALL_BRICK, TOP_STONE);
	addSlopeMountain(42, 42, 2, 0, 0, 8, true, true, true, true, WALL_STONE, TOP_GRASS);

	// Single tall mountain
	addSlopeMountain(50, 10, 4, 0, 0, 10, true, true, true, true, WALL_STONE, TOP_GRASS);

	m_dirty = true;
}

//=============================================================================
void MapEditor::RebuildGeometry(scene::SceneManager& scene)
{
	// Remove old batch nodes from scene
	for (auto& batch : m_batches)
	{
		if (!batch->nodeName.empty())
		{
			scene.root->RemoveChild(batch->nodeName);
		}
	}
	m_batches.clear();

	// Build new batches
	std::vector<MeshBatch> newBatches;

	buildFloorBatches(newBatches);
	buildMountainBatches(newBatches);
	buildMountainRpgMakerBatches(newBatches);

	// Create model nodes for each batch
	for (auto& batch : newBatches)
	{
		if (batch.vertices.empty()) continue;

		batch.material->sampler = m_sampler;

		// Create mesh
		auto mesh = std::make_shared<gr::Mesh>();
		mesh->vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);
		mesh->vbo = gpu::buffer::CreateBuffer(
			batch.vertices.data(),
			batch.vertices.size() * sizeof(gr::MeshVertex));
		mesh->ibo = gpu::buffer::CreateBuffer(
			batch.indices.data(),
			batch.indices.size() * sizeof(uint32_t));
		mesh->vertexCount = static_cast<uint32_t>(batch.vertices.size());
		mesh->indexCount = static_cast<uint32_t>(batch.indices.size());
		mesh->isIndexed = true;

		// Compute AABB
		{
			std::vector<glm::vec3> positions(batch.vertices.size());
			for (size_t i = 0; i < batch.vertices.size(); ++i)
				positions[i] = batch.vertices[i].position;
			mesh->ComputeAABB(positions);
		}

		// Create node
		std::string name = batch.nodeName.empty() ? "batch" : batch.nodeName;
		auto& node = scene.root->AddChild<scene::ModelNode>(name);
		node.mesh = mesh;
		node.material = batch.material;

		// Store batch
		auto batchPtr = std::make_unique<MeshBatch>();
		batchPtr->material = batch.material;
		batchPtr->nodeName = name;
		m_batches.push_back(std::move(batchPtr));
	}

	m_dirty = false;
}

//=============================================================================
void MapEditor::buildFloorBatches(std::vector<MeshBatch>& batches)
{
	auto cellFloorY = [&](int x, int z) -> float
		{
			if (x < 0 || x >= MAP_SIZE || z < 0 || z >= MAP_SIZE) return 0.0f;
			const auto& c = m_grid[z][x];
			return (c.floorTex != FLOOR_NONE) ? c.FloorY() : 0.0f;
		};

	//--- Floor quads (grouped by texture) ---
	{
		struct FloorGroup
		{
			uint8_t texId = 0;
			std::vector<std::pair<int, int>> cells;
		};

		std::array<FloorGroup, FLOOR_COUNT> groups;
		for (uint8_t i = FLOOR_FIRST; i < FLOOR_COUNT; ++i)
			groups[i].texId = i;

		for (int z = 0; z < MAP_SIZE; ++z)
			for (int x = 0; x < MAP_SIZE; ++x)
			{
				uint8_t tex = m_grid[z][x].floorTex;
				if (tex != FLOOR_NONE && tex < FLOOR_COUNT)
					groups[tex].cells.emplace_back(x, z);
			}

		for (uint8_t texId = FLOOR_FIRST; texId < FLOOR_COUNT; ++texId)
		{
			auto& group = groups[texId];
			if (group.cells.empty()) continue;

			MeshBatch batch;
			batch.material = std::make_shared<gr::Material>();
			batch.material->albedoMap = m_floorTex[texId];
			batch.material->albedoColor = glm::vec3(1.0f);
			batch.material->specularColor = glm::vec3(0.1f);
			batch.material->ambientColor = glm::vec3(0.08f);
			batch.material->shininess = 16.0f;

			static int fbc = 0;
			batch.nodeName = "floor_batch_" + std::to_string(fbc++);

			for (auto& [gx, gz] : group.cells)
			{
				float fx = static_cast<float>(gx);
				float fz = static_cast<float>(gz);
				float y  = cellFloorY(gx, gz);

				addQuad(batch,
					{ fx,       y, fz       },
					{ fx,       y, fz + 1.0f },
					{ fx + 1.0f, y, fz + 1.0f },
					{ fx + 1.0f, y, fz       },
					{ 0.0f, 1.0f, 0.0f },
					{ 0.0f, 0.0f },
					{ 0.0f, 1.0f },
					{ 1.0f, 1.0f },
					{ 1.0f, 0.0f });
			}

			batches.push_back(std::move(batch));
		}
	}

	//--- Gap walls between floor cells at different heights ---
	{
		struct WallGroup
		{
			uint8_t   texId = 0;
			MeshBatch batch;
		};

		std::array<WallGroup, WALL_COUNT> wallGroups;
		for (uint8_t i = WALL_FIRST; i < WALL_COUNT; ++i)
		{
			wallGroups[i].texId = i;
			wallGroups[i].batch.material = std::make_shared<gr::Material>();
			wallGroups[i].batch.material->albedoMap = m_wallTex[i];
			wallGroups[i].batch.material->albedoColor = glm::vec3(1.0f);
			wallGroups[i].batch.material->specularColor = glm::vec3(0.3f);
			wallGroups[i].batch.material->ambientColor = glm::vec3(0.08f);
			wallGroups[i].batch.material->shininess = 32.0f;
			wallGroups[i].batch.material->cullMode = gpu::CullMode::None;
			static int fwc = 0;
			wallGroups[i].batch.nodeName = "floor_wall_" + std::to_string(i) + "_" + std::to_string(fwc++);
		}

		for (int z = 0; z < MAP_SIZE; ++z)
			for (int x = 0; x < MAP_SIZE; ++x)
			{
				const auto& cell = m_grid[z][x];
				if (cell.floorTex == FLOOR_NONE) continue;

				float h0 = cell.FloorY();
				uint8_t wallTex = cell.floorWallTex;

				// +X neighbor
				if (x + 1 < MAP_SIZE)
				{
					const auto& nb = m_grid[z][x + 1];
					if (nb.floorTex != FLOOR_NONE)
					{
						float h1 = nb.FloorY();
						if (h0 != h1)
						{
							float lowY  = std::min(h0, h1);
							float highY = std::max(h0, h1);
							float fx1 = static_cast<float>(x + 1);
							float fz  = static_cast<float>(z);
							glm::vec3 normal = (h0 < h1) ? glm::vec3(-1, 0, 0) : glm::vec3(1, 0, 0);
							addWallFace(wallGroups[wallTex].batch,
								{fx1, highY, fz}, {fx1, highY, fz + 1.0f},
								{fx1, lowY,  fz}, {fx1, lowY,  fz + 1.0f},
								normal, highY - lowY);
						}
					}
				}

				// +Z neighbor
				if (z + 1 < MAP_SIZE)
				{
					const auto& nb = m_grid[z + 1][x];
					if (nb.floorTex != FLOOR_NONE)
					{
						float h1 = nb.FloorY();
						if (h0 != h1)
						{
							float lowY  = std::min(h0, h1);
							float highY = std::max(h0, h1);
							float fx  = static_cast<float>(x);
							float fz1 = static_cast<float>(z + 1);
							glm::vec3 normal = (h0 < h1) ? glm::vec3(0, 0, -1) : glm::vec3(0, 0, 1);
							addWallFace(wallGroups[wallTex].batch,
								{fx,     highY, fz1}, {fx + 1.0f, highY, fz1},
								{fx,     lowY,  fz1}, {fx + 1.0f, lowY,  fz1},
								normal, highY - lowY);
						}
					}
				}
			}

		for (auto& wg : wallGroups)
			if (!wg.batch.vertices.empty())
				batches.push_back(std::move(wg.batch));
	}
}

//=============================================================================
void MapEditor::buildMountainBatches(std::vector<MeshBatch>& batches)
{
	// Separate material groups for walls and top faces
	// Walls are grouped by wallTex, tops are grouped by topTex

	struct WallGroup
	{
		uint8_t texId;
		MeshBatch batch;
	};

	struct TopGroup
	{
		uint8_t texId;
		MeshBatch batch;
	};

	std::vector<WallGroup> wallGroups;
	std::vector<TopGroup> topGroups;

	auto findWallGroup = [&](uint8_t texId) -> MeshBatch&
		{
			for (auto& g : wallGroups)
				if (g.texId == texId) return g.batch;

			auto& g = wallGroups.emplace_back();
			g.texId = texId;
			g.batch.material = std::make_shared<gr::Material>();
			g.batch.material->albedoMap = m_wallTex[texId];
			g.batch.material->albedoColor = glm::vec3(1.0f);
			g.batch.material->specularColor = glm::vec3(0.3f);
			g.batch.material->ambientColor = glm::vec3(0.08f);
			g.batch.material->shininess = 32.0f;
			static int wc = 0;
			g.batch.nodeName = "wall_batch_" + std::to_string(wc++);
			return g.batch;
		};

	auto findTopGroup = [&](uint8_t texId) -> MeshBatch&
		{
			for (auto& g : topGroups)
				if (g.texId == texId) return g.batch;

			auto& g = topGroups.emplace_back();
			g.texId = texId;
			g.batch.material = std::make_shared<gr::Material>();
			g.batch.material->albedoMap = m_topTex[texId];
			g.batch.material->albedoColor = glm::vec3(1.0f);
			g.batch.material->specularColor = glm::vec3(0.1f);
			g.batch.material->ambientColor = glm::vec3(0.08f);
			g.batch.material->shininess = 16.0f;
			static int tc = 0;
			g.batch.nodeName = "top_batch_" + std::to_string(tc++);
			return g.batch;
		};

	// Second pass: build geometry
	for (int z = 0; z < MAP_SIZE; ++z)
	{
		for (int x = 0; x < MAP_SIZE; ++x)
		{
			const auto& cell = m_grid[z][x];
			if (cell.mountainStack.empty()) continue;

			// Helper: total top of neighbor cell
			auto neighborTop = [&](int nx, int nz) -> float
				{
					if (nx < 0 || nx >= MAP_SIZE || nz < 0 || nz >= MAP_SIZE)
						return 0.0f;
					return m_grid[nz][nx].MountainTopY();
				};

			float fx = static_cast<float>(x);
			float fz = static_cast<float>(z);

			for (const auto& mt : cell.mountainStack)
			{
				if (mt.mode != MountainMode::Pyramid) continue;

				float baseY = mt.baseY;
				float H = mt.Height();
				float topY = baseY + H;

				float SL = mt.slopeLeft.Total();
				float SR = mt.slopeRight.Total();
				float SF = mt.slopeFront.Total();
				float SB = mt.slopeBack.Total();

				glm::vec3 bot[4] = {
					{fx - SL,       baseY, fz - SB      },
					{fx + 1.0f + SR, baseY, fz - SB      },
					{fx + 1.0f + SR, baseY, fz + 1.0f + SF},
					{fx - SL,       baseY, fz + 1.0f + SF},
				};

				glm::vec3 top[4] = {
					{fx,       topY, fz       },
					{fx + 1.0f, topY, fz       },
					{fx + 1.0f, topY, fz + 1.0f},
					{fx,       topY, fz + 1.0f},
				};

				// Walls — partial culling based on neighbor top
				auto& wallBatch = findWallGroup(mt.texWall);

				// Left wall (-X)
				{
					float adjTop = neighborTop(x - 1, z);
					float wallBot = std::max(baseY, adjTop);
					if (wallBot < topY)
					{
						float visibleH = topY - wallBot;
						glm::vec3 b0 = bot[3]; b0.y = wallBot;
						glm::vec3 b1 = bot[0]; b1.y = wallBot;
						addWallFace(wallBatch, top[3], top[0], b0, b1, { -1.0f, 0.0f, 0.0f }, visibleH);
					}
				}
				// Right wall (+X)
				{
					float adjTop = neighborTop(x + 1, z);
					float wallBot = std::max(baseY, adjTop);
					if (wallBot < topY)
					{
						float visibleH = topY - wallBot;
						glm::vec3 b0 = bot[1]; b0.y = wallBot;
						glm::vec3 b1 = bot[2]; b1.y = wallBot;
						addWallFace(wallBatch, top[1], top[2], b0, b1, { 1.0f, 0.0f, 0.0f }, visibleH);
					}
				}
				// Back wall (-Z)
				{
					float adjTop = neighborTop(x, z - 1);
					float wallBot = std::max(baseY, adjTop);
					if (wallBot < topY)
					{
						float visibleH = topY - wallBot;
						glm::vec3 b0 = bot[0]; b0.y = wallBot;
						glm::vec3 b1 = bot[1]; b1.y = wallBot;
						addWallFace(wallBatch, top[0], top[1], b0, b1, { 0.0f, 0.0f, -1.0f}, visibleH);
					}
				}
				// Front wall (+Z)
				{
					float adjTop = neighborTop(x, z + 1);
					float wallBot = std::max(baseY, adjTop);
					if (wallBot < topY)
					{
						float visibleH = topY - wallBot;
						glm::vec3 b0 = bot[2]; b0.y = wallBot;
						glm::vec3 b1 = bot[3]; b1.y = wallBot;
						addWallFace(wallBatch, top[2], top[3], b0, b1, { 0.0f, 0.0f, 1.0f }, visibleH);
					}
				}

				// Top face
				auto& topBatch = findTopGroup(mt.texTop);
				addQuad(topBatch,
					top[0], top[3], top[2], top[1],
					{ 0.0f, 1.0f, 0.0f },
					{ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f });
			}
		}
	}

	// Add wall groups to batches
	for (auto& g : wallGroups)
	{
		if (g.batch.vertices.empty()) continue;
		batches.push_back(std::move(g.batch));
	}

	// Add top groups to batches
	for (auto& g : topGroups)
	{
		if (g.batch.vertices.empty()) continue;
		batches.push_back(std::move(g.batch));
	}
}

//=============================================================================
void MapEditor::buildMountainRpgMakerBatches(std::vector<MeshBatch>& batches)
{
	struct WallGroup
	{
		uint8_t texId;
		MeshBatch batch;
	};

	struct TopGroup
	{
		uint8_t texId;
		MeshBatch batch;
	};

	std::vector<WallGroup> wallGroups;
	std::vector<TopGroup> topGroups;

	auto findWallGroup = [&](uint8_t texId) -> MeshBatch&
		{
			for (auto& g : wallGroups)
				if (g.texId == texId) return g.batch;

			auto& g = wallGroups.emplace_back();
			g.texId = texId;
			g.batch.material = std::make_shared<gr::Material>();
			g.batch.material->albedoMap = m_wallTex[texId];
			g.batch.material->albedoColor = glm::vec3(1.0f);
			g.batch.material->specularColor = glm::vec3(0.3f);
			g.batch.material->ambientColor = glm::vec3(0.08f);
			g.batch.material->shininess = 32.0f;
			static int wc = 0;
			g.batch.nodeName = "rmwall_batch_" + std::to_string(wc++);
			return g.batch;
		};

	auto findTopGroup = [&](uint8_t texId) -> MeshBatch&
		{
			for (auto& g : topGroups)
				if (g.texId == texId) return g.batch;

			auto& g = topGroups.emplace_back();
			g.texId = texId;
			g.batch.material = std::make_shared<gr::Material>();
			g.batch.material->albedoMap = m_topTex[texId];
			g.batch.material->albedoColor = glm::vec3(1.0f);
			g.batch.material->specularColor = glm::vec3(0.1f);
			g.batch.material->ambientColor = glm::vec3(0.08f);
			g.batch.material->shininess = 16.0f;
			static int tc = 0;
			g.batch.nodeName = "rmtop_batch_" + std::to_string(tc++);
			return g.batch;
		};

	auto neighborTop = [&](int nx, int nz) -> float
		{
			if (nx < 0 || nx >= MAP_SIZE || nz < 0 || nz >= MAP_SIZE)
				return 0.0f;
			return m_grid[nz][nx].MountainTopY();
		};

	for (int z = 0; z < MAP_SIZE; ++z)
		for (int x = 0; x < MAP_SIZE; ++x)
		{
			const auto& cell = m_grid[z][x];
			if (cell.mountainStack.empty()) continue;

			float fx = static_cast<float>(x);
			float fz = static_cast<float>(z);

			for (const auto& mt : cell.mountainStack)
			{
				if (mt.mode != MountainMode::CornerFaces) continue;

				float baseY = mt.baseY;
				float H  = mt.Height();
				float topY = baseY + H;

				float SL = mt.slopeLeft.Total();
				float SR = mt.slopeRight.Total();
				float SF = mt.slopeFront.Total();
				float SB = mt.slopeBack.Total();

				// Adjacent tops in world Y
				float adjTop_L = neighborTop(x - 1, z);
				float adjTop_R = neighborTop(x + 1, z);
				float adjTop_B = neighborTop(x,     z - 1);
				float adjTop_F = neighborTop(x,     z + 1);

				// Wall visible portion: from max(baseY, adjTop) to topY
				float wallBot_L = std::max(baseY, adjTop_L);
				float wallBot_R = std::max(baseY, adjTop_R);
				float wallBot_B = std::max(baseY, adjTop_B);
				float wallBot_F = std::max(baseY, adjTop_F);

				bool showLeft  = wallBot_L < topY;
				bool showRight = wallBot_R < topY;
				bool showBack  = wallBot_B < topY;
				bool showFront = wallBot_F < topY;

				auto& wallBatch = findWallGroup(mt.texWall);

				//--- Left wall (-X) ---
				if (showLeft)
				{
					float visibleH = topY - wallBot_L;
					addWallFace(wallBatch,
						{fx,       topY, fz + 1},
						{fx,       topY, fz    },
						{fx - SL,  wallBot_L, fz + 1},
						{fx - SL,  wallBot_L, fz    },
						{-1, 0, 0}, visibleH);
				}

				//--- Right wall (+X) ---
				if (showRight)
				{
					float visibleH = topY - wallBot_R;
					addWallFace(wallBatch,
						{fx + 1,       topY, fz    },
						{fx + 1,       topY, fz + 1},
						{fx + 1 + SR,  wallBot_R, fz    },
						{fx + 1 + SR,  wallBot_R, fz + 1},
						{1, 0, 0}, visibleH);
				}

				//--- Back wall (-Z) ---
				if (showBack)
				{
					float visibleH = topY - wallBot_B;
					addWallFace(wallBatch,
						{fx,       topY, fz    },
						{fx + 1,   topY, fz    },
						{fx,       wallBot_B, fz - SB},
						{fx + 1,   wallBot_B, fz - SB},
						{0, 0, -1}, visibleH);
				}

				//--- Front wall (+Z) ---
				if (showFront)
				{
					float visibleH = topY - wallBot_F;
					addWallFace(wallBatch,
						{fx + 1,   topY, fz + 1    },
						{fx,       topY, fz + 1    },
						{fx + 1,   wallBot_F, fz + 1 + SF},
						{fx,       wallBot_F, fz + 1 + SF},
						{0, 0, 1}, visibleH);
				}

				//--- Corner triangles ---
				// Back-left
				if (showBack || showLeft)
				{
					uint32_t base = static_cast<uint32_t>(wallBatch.vertices.size());
					glm::vec3 n = glm::normalize(glm::vec3{-1.0f, 0.0f, -1.0f});
					glm::vec3 p0 = {fx - SL, wallBot_L, fz    };
					glm::vec3 p1 = {fx,      wallBot_B, fz - SB};
					glm::vec3 p2 = {fx,      topY,      fz    };
					wallBatch.vertices.push_back({p0, n, {1.0f,    wallBot_L}});
					wallBatch.vertices.push_back({p2, n, {0.5f,    topY     }});
					wallBatch.vertices.push_back({p1, n, {0.0f,    wallBot_B}});
					wallBatch.indices.push_back(base + 0);
					wallBatch.indices.push_back(base + 1);
					wallBatch.indices.push_back(base + 2);
				}
				// Back-right
				if (showBack || showRight)
				{
					uint32_t base = static_cast<uint32_t>(wallBatch.vertices.size());
					glm::vec3 n = glm::normalize(glm::vec3{1.0f, 0.0f, -1.0f});
					glm::vec3 p0 = {fx + 1 + SR, wallBot_R, fz    };
					glm::vec3 p1 = {fx + 1,      wallBot_B, fz - SB};
					glm::vec3 p2 = {fx + 1,      topY,      fz    };
					wallBatch.vertices.push_back({p0, n, {0.0f,    wallBot_R}});
					wallBatch.vertices.push_back({p1, n, {1.0f,    wallBot_B}});
					wallBatch.vertices.push_back({p2, n, {0.5f,    topY     }});
					wallBatch.indices.push_back(base + 0);
					wallBatch.indices.push_back(base + 1);
					wallBatch.indices.push_back(base + 2);
				}
				// Front-right
				if (showFront || showRight)
				{
					uint32_t base = static_cast<uint32_t>(wallBatch.vertices.size());
					glm::vec3 n = glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f});
					glm::vec3 p0 = {fx + 1 + SR, wallBot_R, fz + 1};
					glm::vec3 p1 = {fx + 1,      wallBot_F, fz + 1 + SF};
					glm::vec3 p2 = {fx + 1,      topY,      fz + 1};
					wallBatch.vertices.push_back({p0, n, {1.0f,    wallBot_R}});
					wallBatch.vertices.push_back({p2, n, {0.5f,    topY     }});
					wallBatch.vertices.push_back({p1, n, {0.0f,    wallBot_F}});
					wallBatch.indices.push_back(base + 0);
					wallBatch.indices.push_back(base + 1);
					wallBatch.indices.push_back(base + 2);
				}
				// Front-left
				if (showFront || showLeft)
				{
					uint32_t base = static_cast<uint32_t>(wallBatch.vertices.size());
					glm::vec3 n = glm::normalize(glm::vec3{-1.0f, 0.0f, 1.0f});
					glm::vec3 p0 = {fx - SL, wallBot_L, fz + 1};
					glm::vec3 p1 = {fx,      wallBot_F, fz + 1 + SF};
					glm::vec3 p2 = {fx,      topY,      fz + 1};
					wallBatch.vertices.push_back({p0, n, {0.0f,    wallBot_L}});
					wallBatch.vertices.push_back({p1, n, {1.0f,    wallBot_F}});
					wallBatch.vertices.push_back({p2, n, {0.5f,    topY     }});
					wallBatch.indices.push_back(base + 0);
					wallBatch.indices.push_back(base + 1);
					wallBatch.indices.push_back(base + 2);
				}

				//--- Top face: flat quad ---
				auto& topBatch = findTopGroup(mt.texTop);
				addQuad(topBatch,
					{fx,       topY, fz      },
					{fx,       topY, fz + 1  },
					{fx + 1,   topY, fz + 1  },
					{fx + 1,   topY, fz      },
					{0, 1, 0},
					{0, 0},
					{0, 1},
					{1, 1},
					{1, 0});
			}
		}

	for (auto& g : wallGroups)
		if (!g.batch.vertices.empty())
			batches.push_back(std::move(g.batch));

	for (auto& g : topGroups)
		if (!g.batch.vertices.empty())
			batches.push_back(std::move(g.batch));
}

//=============================================================================
void MapEditor::addQuad(
	MeshBatch& batch,
	glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
	glm::vec3 n,
	glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3) const
{
	uint32_t base = static_cast<uint32_t>(batch.vertices.size());

	batch.vertices.push_back({ .position = v0, .normal = n, .uv = uv0 });
	batch.vertices.push_back({ .position = v1, .normal = n, .uv = uv1 });
	batch.vertices.push_back({ .position = v2, .normal = n, .uv = uv2 });
	batch.vertices.push_back({ .position = v3, .normal = n, .uv = uv3 });

	batch.indices.push_back(base + 0);
	batch.indices.push_back(base + 1);
	batch.indices.push_back(base + 2);
	batch.indices.push_back(base + 2);
	batch.indices.push_back(base + 3);
	batch.indices.push_back(base + 0);
}

//=============================================================================
void MapEditor::addWallFace(
	MeshBatch& batch,
	glm::vec3 t0, glm::vec3 t1, // top edge (t0→t1)
	glm::vec3 b0, glm::vec3 b1, // bottom edge (b0→b1)
	glm::vec3 normal,
	float texRepeatV,
	int subdivs) const
{
	// Edge lengths for centred UV
	glm::vec3 topDir = t1 - t0;
	glm::vec3 botDir = b1 - b0;
	float topLen = glm::length(topDir);
	float botLen = glm::length(botDir);

	// Centred UV: U=0.5 at the wall midpoint, expands symmetrically
	float uTopStart = 0.5f - topLen * 0.5f;
	float uTopEnd   = 0.5f + topLen * 0.5f;
	float uBotStart = 0.5f - botLen * 0.5f;
	float uBotEnd   = 0.5f + botLen * 0.5f;

	// Split the wall into horizontal bands to minimise texture skew
	for (int i = 0; i < subdivs; ++i)
	{
		float f0 = static_cast<float>(i)     / static_cast<float>(subdivs);
		float f1 = static_cast<float>(i + 1) / static_cast<float>(subdivs);

		// Interpolate vertex positions for this band
		glm::vec3 band_t0 = b0 + (t0 - b0) * f1;
		glm::vec3 band_t1 = b1 + (t1 - b1) * f1;
		glm::vec3 band_b0 = b0 + (t0 - b0) * f0;
		glm::vec3 band_b1 = b1 + (t1 - b1) * f0;

		// Interpolate U for this band, V for this band
		float uBandTopStart = uBotStart + (uTopStart - uBotStart) * f1;
		float uBandTopEnd   = uBotEnd   + (uTopEnd   - uBotEnd)   * f1;
		float uBandBotStart = uBotStart + (uTopStart - uBotStart) * f0;
		float uBandBotEnd   = uBotEnd   + (uTopEnd   - uBotEnd)   * f0;
		float vTop = f1 * texRepeatV;
		float vBot = f0 * texRepeatV;

		uint32_t base = static_cast<uint32_t>(batch.vertices.size());

		// Auto-correct winding per band (band is nearly a rectangle
		// so computedNormal should be consistent)
		glm::vec3 computedNormal = glm::cross(band_b0 - band_t0, band_b1 - band_t0);
		if (glm::dot(computedNormal, normal) >= 0.0f)
		{
			batch.vertices.push_back({ .position = band_t0, .normal = normal, .uv = {uBandTopStart, vTop} });
			batch.vertices.push_back({ .position = band_b0, .normal = normal, .uv = {uBandBotStart, vBot} });
			batch.vertices.push_back({ .position = band_b1, .normal = normal, .uv = {uBandBotEnd, vBot} });
			batch.vertices.push_back({ .position = band_t1, .normal = normal, .uv = {uBandTopEnd, vTop} });
		}
		else
		{
			batch.vertices.push_back({ .position = band_t0, .normal = normal, .uv = {uBandTopStart, vTop} });
			batch.vertices.push_back({ .position = band_t1, .normal = normal, .uv = {uBandTopEnd, vTop} });
			batch.vertices.push_back({ .position = band_b1, .normal = normal, .uv = {uBandBotEnd, vBot} });
			batch.vertices.push_back({ .position = band_b0, .normal = normal, .uv = {uBandBotStart, vBot} });
		}

		batch.indices.push_back(base + 0);
		batch.indices.push_back(base + 1);
		batch.indices.push_back(base + 2);
		batch.indices.push_back(base + 2);
		batch.indices.push_back(base + 3);
		batch.indices.push_back(base + 0);
	}
}

//=============================================================================
void MapEditor::Update(scene::SceneManager& scene)
{
	// Rebuild geometry if dirty
	if (m_dirty)
	{
		RebuildGeometry(scene);
	}

	// Ctrl height capture: on first frame Ctrl is held, capture Y at hover cell
	bool ctrlDown = input::IsKeyDown(KeyboardType::KEY_LEFT_CONTROL)
		|| input::IsKeyDown(KeyboardType::KEY_RIGHT_CONTROL);
	if (ctrlDown)
	{
		if (!m_state.ctrlHeightLocked)
		{
			m_state.ctrlHeightLocked = true;
			int gx = m_state.hoverGridX;
			int gz = m_state.hoverGridZ;
			if (gx >= 0 && gx < MAP_SIZE && gz >= 0 && gz < MAP_SIZE)
			{
				const auto& cell = m_grid[gz][gx];
				m_state.ctrlBaseY = cell.MountainTopY();
				if (!cell.mountainStack.empty())
				{
					// already set by MountainTopY()
				}
				else if (cell.floorTex != FLOOR_NONE)
					m_state.ctrlBaseY = cell.FloorY();
				else
					m_state.ctrlBaseY = 0.0f;
			}
		}
	}
	else
	{
		m_state.ctrlHeightLocked = false;
	}

	// Track stroke: increment stroke ID on each LMB press
	static bool s_prevMouseDown = false;
	bool mouseDown = input::IsMouseDown( MouseType::MOUSE_BUTTON_LEFT );
	if (mouseDown && !s_prevMouseDown)
		++m_strokeId;
	s_prevMouseDown = mouseDown;
}

//=============================================================================
void MapEditor::RenderUI()
{
	ImGui::Begin("Map Editor");

	// Tool selection
	const char* toolNames[] = { "Floor Brush", "Mountain Brush", "Eraser" };
	int currentTool = static_cast<int>(m_state.activeTool);
	if (ImGui::Combo("Tool", &currentTool, toolNames, IM_ARRAYSIZE(toolNames)))
		m_state.activeTool = static_cast<EditorTool>(currentTool);

	ImGui::Separator();

	if (m_state.activeTool == EditorTool::FloorBrush)
	{
		const char* floorNames[] = { "Grass", "Dirt", "Stone", "Sand", "Water", "Wood" };
		int currentFloor = static_cast<int>(m_state.selectedFloorTex) - 1;
		if (ImGui::Combo("Floor Texture", &currentFloor, floorNames, IM_ARRAYSIZE(floorNames)))
			m_state.selectedFloorTex = static_cast<uint8_t>(currentFloor + 1);

		// Floor height (-5..5, 5 = 0.5 blocks)
		int floorH = m_state.floorHeight;
		ImGui::SliderInt("Height", &floorH, FLOOR_HEIGHT_MIN, FLOOR_HEIGHT_MAX);
		m_state.floorHeight = static_cast<int8_t>(std::clamp(floorH, FLOOR_HEIGHT_MIN, FLOOR_HEIGHT_MAX));

		float floorWorldY = static_cast<float>(m_state.floorHeight) * FLOOR_HEIGHT_SCALE;
		ImGui::Text("World Y: %.2f", floorWorldY);

		ImGui::Separator();
		ImGui::Text("Gap Walls");

		const char* wallNames[] = { "Stone", "Brick", "Dirt" };
		int currentFloorWall = static_cast<int>(m_state.selectedFloorWallTex) - 1;
		if (ImGui::Combo("Wall Texture", &currentFloorWall, wallNames, IM_ARRAYSIZE(wallNames)))
			m_state.selectedFloorWallTex = static_cast<uint8_t>(currentFloorWall + 1);
	}

	if (m_state.activeTool == EditorTool::MountainBrush)
	{
		// Render mode
		const char* modeNames[] = { "Pyramid", "Corner Faces" };
		int currentMode = static_cast<int>(m_state.mountainMode);
		if (ImGui::Combo("Mode", &currentMode, modeNames, IM_ARRAYSIZE(modeNames)))
			m_state.mountainMode = static_cast<MountainMode>(currentMode);

		const char* wallNames[] = { "Stone", "Brick", "Dirt" };
		int currentWall = static_cast<int>(m_state.selectedWallTex) - 1;
		if (ImGui::Combo("Wall Texture", &currentWall, wallNames, IM_ARRAYSIZE(wallNames)))
			m_state.selectedWallTex = static_cast<uint8_t>(currentWall + 1);

		const char* topNames[] = { "Grass", "Stone", "Dirt" };
		int currentTop = static_cast<int>(m_state.selectedTopTex) - 1;
		if (ImGui::Combo("Top Texture", &currentTop, topNames, IM_ARRAYSIZE(topNames)))
			m_state.selectedTopTex = static_cast<uint8_t>(currentTop + 1);

		ImGui::Separator();
		ImGui::Text("Height");

		int hBlocks = m_state.mountainHeightBlocks;
		ImGui::InputInt("Blocks", &hBlocks);
		if (hBlocks < 1) hBlocks = 1;
		if (hBlocks > 100) hBlocks = 100;
		m_state.mountainHeightBlocks = static_cast<int16_t>(hBlocks);

		int hPixels = m_state.mountainHeightPixels;
		ImGui::InputInt("Pixels (0-16)", &hPixels);
		if (hPixels < 0) hPixels = 0;
		if (hPixels > 16) hPixels = 16;
		m_state.mountainHeightPixels = static_cast<int16_t>(hPixels);

		ImGui::Separator();
		ImGui::Text("Slope Frame");

		int sBlocks = m_state.slopeBlocks;
		ImGui::InputInt("Slope blocks", &sBlocks);
		if (sBlocks < 0) sBlocks = 0;
		if (sBlocks > 10) sBlocks = 10;
		m_state.slopeBlocks = static_cast<int16_t>(sBlocks);

		int sPixels = m_state.slopePixels;
		ImGui::InputInt("Slope pixels (0-16)", &sPixels);
		if (sPixels < 0) sPixels = 0;
		if (sPixels > 16) sPixels = 16;
		m_state.slopePixels = static_cast<int16_t>(sPixels);

		ImGui::Checkbox("Left side", &m_state.slopeLeft);
		ImGui::Checkbox("Right side", &m_state.slopeRight);
		ImGui::Checkbox("Front side", &m_state.slopeFront);
		ImGui::Checkbox("Back side", &m_state.slopeBack);

		ImGui::Separator();
		float totalSlope = static_cast<float>(m_state.slopeBlocks) +
			static_cast<float>(m_state.slopePixels) / 16.0f;
		ImGui::Text("Total slope: %.2f", totalSlope);

		ImGui::Separator();
		float totalHeight = static_cast<float>(m_state.mountainHeightBlocks) +
			static_cast<float>(m_state.mountainHeightPixels) / 16.0f;
		ImGui::Text("Total height: %.2f", totalHeight);
	}

	ImGui::Separator();

	// Paint with left mouse button
	if (m_state.hoverGridX >= 0 && m_state.hoverGridX < MAP_SIZE &&
		m_state.hoverGridZ >= 0 && m_state.hoverGridZ < MAP_SIZE)
	{
		ImGui::Text("Hover: (%d, %d)", m_state.hoverGridX, m_state.hoverGridZ);

		if (input::IsMouseDown(MouseType::MOUSE_BUTTON_LEFT) &&
			!ImGui::GetIO().WantCaptureMouse)
		{
			if (m_state.activeTool == EditorTool::Eraser)
				EraseCell(m_state.hoverGridX, m_state.hoverGridZ);
			else
				PaintCell(m_state.hoverGridX, m_state.hoverGridZ);
		}
	}

	ImGui::End();
}

//=============================================================================
const MapCell& MapEditor::GetCell(int gx, int gz) const
{
	gz = std::clamp(gz, 0, MAP_SIZE - 1);
	gx = std::clamp(gx, 0, MAP_SIZE - 1);
	return m_grid[gz][gx];
}

//=============================================================================
MapCell& MapEditor::GetCell(int gx, int gz)
{
	gz = std::clamp(gz, 0, MAP_SIZE - 1);
	gx = std::clamp(gx, 0, MAP_SIZE - 1);
	return m_grid[gz][gx];
}

//=============================================================================
bool MapEditor::ScreenToGrid(int sx, int sy, int& outGx, int& outGz) const
{
	// Get viewport size
	float vpW = static_cast<float>(window::GetWidth());
	float vpH = static_cast<float>(window::GetHeight());

	// Convert to NDC [-1, 1]
	float ndcX = (2.0f * static_cast<float>(sx) / vpW - 1.0f);
	float ndcY = (1.0f - 2.0f * static_cast<float>(sy) / vpH);

	// Get view-projection matrix
	// We need camera info - but we don't have it directly here.
	// Instead, we compute the ray in world space using the inverse VP matrix
	// that the scene manager would have computed.

	// This is a placeholder - the actual raycast is done in EditorApp
	// which has access to the camera.
	return false;
}

//=============================================================================
void MapEditor::PaintCell(int gx, int gz)
{
	if (gx < 0 || gx >= MAP_SIZE || gz < 0 || gz >= MAP_SIZE)
		return;

	auto& cell = m_grid[gz][gx];

	if (m_state.activeTool == EditorTool::FloorBrush)
	{
		cell.floorTex     = m_state.selectedFloorTex;
		cell.floorHeight  = m_state.floorHeight;
		cell.floorWallTex = m_state.selectedFloorWallTex;
	}
	else if (m_state.activeTool == EditorTool::MountainBrush)
	{
		// Skip if already painted in this stroke (prevents infinite stacking on mouse-hold)
		if (m_cellStrokeId[gz][gx] == m_strokeId)
			return;

		if (m_state.ctrlHeightLocked)
		{
			// Ctrl: clear stack, place fresh mountain at captured Y
			MountainData candidate;
			candidate.hasMountain  = true;
			candidate.baseY        = m_state.ctrlBaseY;
			candidate.mode         = m_state.mountainMode;
			candidate.heightBlocks = static_cast<int16_t>(m_state.mountainHeightBlocks);
			candidate.heightPixels = static_cast<int16_t>(m_state.mountainHeightPixels);
			candidate.texWall      = m_state.selectedWallTex;
			candidate.texTop       = m_state.selectedTopTex;
			candidate.slopeLeft.blocks   = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeLeft.pixels   = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopePixels) : 0;
			candidate.slopeRight.blocks  = m_state.slopeRight ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeRight.pixels  = m_state.slopeRight ? static_cast<int16_t>(m_state.slopePixels) : 0;
			candidate.slopeFront.blocks  = m_state.slopeFront ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeFront.pixels  = m_state.slopeFront ? static_cast<int16_t>(m_state.slopePixels) : 0;
			candidate.slopeBack.blocks   = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeBack.pixels   = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopePixels) : 0;

			// Check if the cell already has an identical single mountain
			auto isSame = []( const MountainData& a, const MountainData& b ) noexcept -> bool
				{
					if (a.hasMountain != b.hasMountain) return false;
					if (!a.hasMountain) return true;
					return a.mode == b.mode
						&& a.baseY == b.baseY
						&& a.heightBlocks == b.heightBlocks
						&& a.heightPixels == b.heightPixels
						&& a.texWall == b.texWall
						&& a.texTop == b.texTop
						&& a.slopeLeft.blocks  == b.slopeLeft.blocks  && a.slopeLeft.pixels  == b.slopeLeft.pixels
						&& a.slopeRight.blocks == b.slopeRight.blocks && a.slopeRight.pixels == b.slopeRight.pixels
						&& a.slopeFront.blocks == b.slopeFront.blocks && a.slopeFront.pixels == b.slopeFront.pixels
						&& a.slopeBack.blocks  == b.slopeBack.blocks  && a.slopeBack.pixels  == b.slopeBack.pixels;
				};

			bool same = (cell.mountainStack.size() == 1)
				&& isSame( cell.mountainStack.back(), candidate );

			if (!same)
			{
				cell.mountainStack.clear();
				cell.mountainStack.push_back( candidate );
				m_cellStrokeId[gz][gx] = m_strokeId;
			}
		}
		else if (!cell.mountainStack.empty())
		{
			auto& top = cell.mountainStack.back();

			// Check if top layer has the same visual properties (mode, textures, slopes)
			bool sameProperties = (top.mode == m_state.mountainMode)
				&& (top.texWall == m_state.selectedWallTex)
				&& (top.texTop == m_state.selectedTopTex)
				&& (top.slopeLeft.blocks  == (m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopeBlocks) : 0))
				&& (top.slopeLeft.pixels  == (m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopePixels) : 0))
				&& (top.slopeRight.blocks == (m_state.slopeRight ? static_cast<int16_t>(m_state.slopeBlocks) : 0))
				&& (top.slopeRight.pixels == (m_state.slopeRight ? static_cast<int16_t>(m_state.slopePixels) : 0))
				&& (top.slopeFront.blocks == (m_state.slopeFront ? static_cast<int16_t>(m_state.slopeBlocks) : 0))
				&& (top.slopeFront.pixels == (m_state.slopeFront ? static_cast<int16_t>(m_state.slopePixels) : 0))
				&& (top.slopeBack.blocks  == (m_state.slopeBack  ? static_cast<int16_t>(m_state.slopeBlocks) : 0))
				&& (top.slopeBack.pixels  == (m_state.slopeBack  ? static_cast<int16_t>(m_state.slopePixels) : 0));

			if (sameProperties)
			{
				// Merge: grow the top layer
				float placedH = static_cast<float>(m_state.mountainHeightBlocks)
					+ static_cast<float>(m_state.mountainHeightPixels) / 16.0f;
				float newH = top.Height() + placedH;
				top.heightBlocks = static_cast<int16_t>( static_cast<int>( newH ));
				top.heightPixels = static_cast<int16_t>(
					( newH - static_cast<float>( static_cast<int>( newH ))) * 16.0f + 0.5f );
			}
			else
			{
				// Different properties: push a new layer on top
				MountainData layer;
				layer.hasMountain  = true;
				layer.baseY        = top.baseY + top.Height();
				layer.mode         = m_state.mountainMode;
				layer.heightBlocks = static_cast<int16_t>(m_state.mountainHeightBlocks);
				layer.heightPixels = static_cast<int16_t>(m_state.mountainHeightPixels);
				layer.texWall      = m_state.selectedWallTex;
				layer.texTop       = m_state.selectedTopTex;
				layer.slopeLeft.blocks   = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
				layer.slopeLeft.pixels   = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopePixels) : 0;
				layer.slopeRight.blocks  = m_state.slopeRight ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
				layer.slopeRight.pixels  = m_state.slopeRight ? static_cast<int16_t>(m_state.slopePixels) : 0;
				layer.slopeFront.blocks  = m_state.slopeFront ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
				layer.slopeFront.pixels  = m_state.slopeFront ? static_cast<int16_t>(m_state.slopePixels) : 0;
				layer.slopeBack.blocks   = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
				layer.slopeBack.pixels   = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopePixels) : 0;
				cell.mountainStack.push_back( layer );
			}

			m_cellStrokeId[gz][gx] = m_strokeId;
		}
		else
		{
			// Fresh mountain on empty cell
			float baseY = (cell.floorTex != FLOOR_NONE) ? cell.FloorY() : 0.0f;

			MountainData candidate;
			candidate.hasMountain  = true;
			candidate.baseY        = baseY;
			candidate.mode         = m_state.mountainMode;
			candidate.heightBlocks = static_cast<int16_t>(m_state.mountainHeightBlocks);
			candidate.heightPixels = static_cast<int16_t>(m_state.mountainHeightPixels);
			candidate.texWall      = m_state.selectedWallTex;
			candidate.texTop       = m_state.selectedTopTex;
			candidate.slopeLeft.blocks   = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeLeft.pixels   = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopePixels) : 0;
			candidate.slopeRight.blocks  = m_state.slopeRight ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeRight.pixels  = m_state.slopeRight ? static_cast<int16_t>(m_state.slopePixels) : 0;
			candidate.slopeFront.blocks  = m_state.slopeFront ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeFront.pixels  = m_state.slopeFront ? static_cast<int16_t>(m_state.slopePixels) : 0;
			candidate.slopeBack.blocks   = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
			candidate.slopeBack.pixels   = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopePixels) : 0;

			cell.mountainStack.push_back( candidate );
			m_cellStrokeId[gz][gx] = m_strokeId;
		}
	}

	m_dirty = true;
}

//=============================================================================
void MapEditor::EraseCell(int gx, int gz)
{
	if (gx < 0 || gx >= MAP_SIZE || gz < 0 || gz >= MAP_SIZE)
		return;

	auto& cell = m_grid[gz][gx];

	if (m_state.activeTool == EditorTool::Eraser)
	{
		// Erase everything
		cell = MapCell{};
	}
	else if (m_state.activeTool == EditorTool::FloorBrush)
	{
		cell.floorTex = FLOOR_NONE;
	}
	else if (m_state.activeTool == EditorTool::MountainBrush)
	{
		cell.mountainStack.clear();
	}

	m_dirty = true;
}

//=============================================================================
bool MapEditor::BuildGhostPreview(
	int gx, int gz,
	std::vector<gr::MeshVertex>& verts,
	std::vector<uint32_t>& indices) const
{
	if (gx < 0 || gx >= MAP_SIZE || gz < 0 || gz >= MAP_SIZE)
		return false;

	verts.clear();
	indices.clear();

	if (m_state.activeTool == EditorTool::Eraser)
		return false;

	if (m_state.activeTool == EditorTool::FloorBrush)
	{
		float fx = static_cast<float>(gx);
		float fz = static_cast<float>(gz);
		float h  = static_cast<float>(m_state.floorHeight) * FLOOR_HEIGHT_SCALE + 0.01f;
		glm::vec3 n = { 0.0f, 1.0f, 0.0f };
		uint32_t base = 0;

		verts.push_back({ .position = {fx,     h, fz     }, .normal = n, .uv = {0, 0} });
		verts.push_back({ .position = {fx,     h, fz + 1}, .normal = n, .uv = {0, 1} });
		verts.push_back({ .position = {fx + 1, h, fz + 1}, .normal = n, .uv = {1, 1} });
		verts.push_back({ .position = {fx + 1, h, fz     }, .normal = n, .uv = {1, 0} });

		indices.push_back(base + 0);
		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
		indices.push_back(base + 0);
		return true;
	}

	if (m_state.activeTool == EditorTool::MountainBrush)
	{
		float fx = static_cast<float>(gx);
		float fz = static_cast<float>(gz);

		// Build a temporary MountainData from editor state
		MountainData mt;
		mt.hasMountain = true;
		mt.heightBlocks = m_state.mountainHeightBlocks;
		mt.heightPixels = m_state.mountainHeightPixels;
		mt.texWall = m_state.selectedWallTex;
		mt.texTop = m_state.selectedTopTex;

		// Determine baseY for the ghost:
		//   Ctrl → fixed captured Y
		//   otherwise → stack on top of whatever is in the hover cell
		{
			const auto& cell = m_grid[gz][gx];
			if (m_state.ctrlHeightLocked)
			{
				mt.baseY = m_state.ctrlBaseY;
			}
			else if (!cell.mountainStack.empty() && cell.mountainStack.back().mode == m_state.mountainMode)
			{
				// Ghost shows the stacking result: on top of the existing top layer
				const auto& top = cell.mountainStack.back();
				mt.baseY = top.baseY + top.Height();
			}
			else if (cell.floorTex != FLOOR_NONE)
			{
				mt.baseY = cell.FloorY();
			}
			else
			{
				mt.baseY = 0.0f;
			}
		}

		MeshBatch tmp;
		tmp.material = nullptr;

		float baseY = mt.baseY;
		float H = mt.Height();
		float topY = baseY + H;

		if (m_state.mountainMode == MountainMode::CornerFaces)
		{
			mt.slopeLeft.blocks   = m_state.slopeLeft  ? m_state.slopeBlocks : 0;
			mt.slopeLeft.pixels   = m_state.slopeLeft  ? m_state.slopePixels : 0;
			mt.slopeRight.blocks  = m_state.slopeRight ? m_state.slopeBlocks : 0;
			mt.slopeRight.pixels  = m_state.slopeRight ? m_state.slopePixels : 0;
			mt.slopeFront.blocks  = m_state.slopeFront ? m_state.slopeBlocks : 0;
			mt.slopeFront.pixels  = m_state.slopeFront ? m_state.slopePixels : 0;
			mt.slopeBack.blocks   = m_state.slopeBack  ? m_state.slopeBlocks : 0;
			mt.slopeBack.pixels   = m_state.slopeBack  ? m_state.slopePixels : 0;

			float SL = mt.slopeLeft.Total();
			float SR = mt.slopeRight.Total();
			float SF = mt.slopeFront.Total();
			float SB = mt.slopeBack.Total();

			//--- Walls: only extend in own normal direction ---
			addWallFace(tmp, {fx,     topY, fz + 1}, {fx,     topY, fz    },
				{fx - SL, baseY, fz + 1}, {fx - SL, baseY, fz    }, {-1, 0, 0}, H);
			addWallFace(tmp, {fx + 1, topY, fz    }, {fx + 1, topY, fz + 1},
				{fx + 1 + SR, baseY, fz    }, {fx + 1 + SR, baseY, fz + 1}, {1, 0, 0}, H);
			addWallFace(tmp, {fx,     topY, fz    }, {fx + 1, topY, fz    },
				{fx,     baseY, fz - SB}, {fx + 1, baseY, fz - SB}, {0, 0, -1}, H);
			addWallFace(tmp, {fx + 1, topY, fz + 1}, {fx,     topY, fz + 1},
				{fx + 1, baseY, fz + 1 + SF}, {fx, baseY, fz + 1 + SF}, {0, 0, 1}, H);

			//--- Corner triangles ---
			// UV convention: U=0..1 along the wall, V = world-space Y.
			// Winding is CCW when viewed from the outward diagonal.

			// Back-left
			{
				uint32_t base = static_cast<uint32_t>(tmp.vertices.size());
				glm::vec3 n = glm::normalize(glm::vec3{-1.0f, 0.0f, -1.0f});
				tmp.vertices.push_back({{fx - SL, baseY, fz      }, n, {1.0f, baseY}});
				tmp.vertices.push_back({{fx,      topY, fz      }, n, {0.5f, topY}});
				tmp.vertices.push_back({{fx,      baseY, fz - SB }, n, {0.0f, baseY}});
				tmp.indices.push_back(base + 0);
				tmp.indices.push_back(base + 1);
				tmp.indices.push_back(base + 2);
			}

			// Back-right
			{
				uint32_t base = static_cast<uint32_t>(tmp.vertices.size());
				glm::vec3 n = glm::normalize(glm::vec3{1.0f, 0.0f, -1.0f});
				tmp.vertices.push_back({{fx + 1 + SR, baseY, fz      }, n, {0.0f, baseY}});
				tmp.vertices.push_back({{fx + 1,      baseY, fz - SB }, n, {1.0f, baseY}});
				tmp.vertices.push_back({{fx + 1,      topY, fz      }, n, {0.5f, topY}});
				tmp.indices.push_back(base + 0);
				tmp.indices.push_back(base + 1);
				tmp.indices.push_back(base + 2);
			}

			// Front-right
			{
				uint32_t base = static_cast<uint32_t>(tmp.vertices.size());
				glm::vec3 n = glm::normalize(glm::vec3{1.0f, 0.0f, 1.0f});
				tmp.vertices.push_back({{fx + 1 + SR, baseY, fz + 1  }, n, {1.0f, baseY}});
				tmp.vertices.push_back({{fx + 1,      topY, fz + 1  }, n, {0.5f, topY}});
				tmp.vertices.push_back({{fx + 1,      baseY, fz + 1 + SF}, n, {0.0f, baseY}});
				tmp.indices.push_back(base + 0);
				tmp.indices.push_back(base + 1);
				tmp.indices.push_back(base + 2);
			}

			// Front-left
			{
				uint32_t base = static_cast<uint32_t>(tmp.vertices.size());
				glm::vec3 n = glm::normalize(glm::vec3{-1.0f, 0.0f, 1.0f});
				tmp.vertices.push_back({{fx - SL, baseY, fz + 1  }, n, {0.0f, baseY}});
				tmp.vertices.push_back({{fx,      baseY, fz + 1 + SF}, n, {1.0f, baseY}});
				tmp.vertices.push_back({{fx,      topY, fz + 1  }, n, {0.5f, topY}});
				tmp.indices.push_back(base + 0);
				tmp.indices.push_back(base + 1);
				tmp.indices.push_back(base + 2);
			}

			//--- Top face: flat quad ---
			addQuad(tmp,
				{fx,       topY, fz      },
				{fx,       topY, fz + 1  },
				{fx + 1,   topY, fz + 1  },
				{fx + 1,   topY, fz      },
				{0, 1, 0},
				{0, 0}, {0, 1}, {1, 1}, {1, 0});
		}
		else
		{
			mt.slopeLeft.blocks   = m_state.slopeLeft  ? m_state.slopeBlocks : 0;
			mt.slopeLeft.pixels   = m_state.slopeLeft  ? m_state.slopePixels : 0;
			mt.slopeRight.blocks  = m_state.slopeRight ? m_state.slopeBlocks : 0;
			mt.slopeRight.pixels  = m_state.slopeRight ? m_state.slopePixels : 0;
			mt.slopeFront.blocks  = m_state.slopeFront ? m_state.slopeBlocks : 0;
			mt.slopeFront.pixels  = m_state.slopeFront ? m_state.slopePixels : 0;
			mt.slopeBack.blocks   = m_state.slopeBack  ? m_state.slopeBlocks : 0;
			mt.slopeBack.pixels   = m_state.slopeBack  ? m_state.slopePixels : 0;

			float SL = mt.slopeLeft.Total();
			float SR = mt.slopeRight.Total();
			float SF = mt.slopeFront.Total();
			float SB = mt.slopeBack.Total();

			// Corner vertices at y=baseY and y=topY (no partial culling — show full wall)
			glm::vec3 bot[4] = {
				{fx - SL,       baseY, fz - SB      },
				{fx + 1.0f + SR, baseY, fz - SB      },
				{fx + 1.0f + SR, baseY, fz + 1.0f + SF},
				{fx - SL,       baseY, fz + 1.0f + SF},
			};
			glm::vec3 top[4] = {
				{fx,       topY, fz       },
				{fx + 1.0f, topY, fz       },
				{fx + 1.0f, topY, fz + 1.0f},
				{fx,       topY, fz + 1.0f},
			};

			// Walls (no neighbour culling — show full height)
			addWallFace(tmp, top[3], top[0], bot[3], bot[0], { -1.0f, 0.0f, 0.0f }, H);
			addWallFace(tmp, top[1], top[2], bot[1], bot[2], {  1.0f, 0.0f, 0.0f }, H);
			addWallFace(tmp, top[0], top[1], bot[0], bot[1], {  0.0f, 0.0f, -1.0f}, H);
			addWallFace(tmp, top[2], top[3], bot[2], bot[3], {  0.0f, 0.0f,  1.0f}, H);

			// Top face (CCW from above: LB -> LF -> RF -> RB)
			addQuad(tmp, top[0], top[3], top[2], top[1],
				{ 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f });
		}

		verts = std::move(tmp.vertices);
		indices = std::move(tmp.indices);
		return true;
	}

	return false;
}

} //namespace map
