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
			cell.mountain.hasMountain = true;
			cell.mountain.heightBlocks = static_cast<int16_t>(heightBlocks);
			cell.mountain.heightPixels = static_cast<int16_t>(heightPixels);
			cell.mountain.texWall = wallTex;
			cell.mountain.texTop = topTex;
		};

	// Mountain with slope
	auto addSlopeMountain = [&](int gx, int gz,
		int heightBlocks, int heightPixels,
		int slopeBlocks, int slopePixels,
		bool slopeL, bool slopeR, bool slopeF, bool slopeB,
		uint8_t wallTex, uint8_t topTex)
		{
			auto& cell = m_grid[gz][gx];
			cell.mountain.hasMountain = true;
			cell.mountain.heightBlocks = static_cast<int16_t>(heightBlocks);
			cell.mountain.heightPixels = static_cast<int16_t>(heightPixels);
			cell.mountain.texWall = wallTex;
			cell.mountain.texTop = topTex;
			if (slopeL) { cell.mountain.slopeLeft.blocks = static_cast<int16_t>(slopeBlocks);
				cell.mountain.slopeLeft.pixels = static_cast<int16_t>(slopePixels); }
			if (slopeR) { cell.mountain.slopeRight.blocks = static_cast<int16_t>(slopeBlocks);
				cell.mountain.slopeRight.pixels = static_cast<int16_t>(slopePixels); }
			if (slopeF) { cell.mountain.slopeFront.blocks = static_cast<int16_t>(slopeBlocks);
				cell.mountain.slopeFront.pixels = static_cast<int16_t>(slopePixels); }
			if (slopeB) { cell.mountain.slopeBack.blocks = static_cast<int16_t>(slopeBlocks);
				cell.mountain.slopeBack.pixels = static_cast<int16_t>(slopePixels); }
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
	// Group floor cells by texture type
	struct FloorGroup
	{
		uint8_t texId = 0;
		std::vector<std::pair<int, int>> cells; // (gx, gz) pairs
	};

	std::array<FloorGroup, FLOOR_COUNT> groups;
	for (uint8_t i = FLOOR_FIRST; i < FLOOR_COUNT; ++i)
		groups[i].texId = i;

	for (int z = 0; z < MAP_SIZE; ++z)
	{
		for (int x = 0; x < MAP_SIZE; ++x)
		{
			uint8_t tex = m_grid[z][x].floorTex;
			if (tex != FLOOR_NONE && tex < FLOOR_COUNT)
				groups[tex].cells.emplace_back(x, z);
		}
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

		static int floorBatchCounter = 0;
		batch.nodeName = "floor_batch_" + std::to_string(floorBatchCounter++);

		for (auto& [gx, gz] : group.cells)
		{
			float fx = static_cast<float>(gx);
			float fz = static_cast<float>(gz);

			// CCW from above: LB → LF → RF → RB
			addQuad(batch,
				{ fx,       0.0f, fz       }, // v0 = left-back
				{ fx,       0.0f, fz + 1.0f }, // v1 = left-front
				{ fx + 1.0f, 0.0f, fz + 1.0f }, // v2 = right-front
				{ fx + 1.0f, 0.0f, fz       }, // v3 = right-back
				{ 0.0f, 1.0f, 0.0f },           // normal up
				{ 0.0f, 0.0f },                  // uv0 (LB)
				{ 0.0f, 1.0f },                  // uv1 (LF)
				{ 1.0f, 1.0f },                  // uv2 (RF)
				{ 1.0f, 0.0f });                 // uv3 (RB)
		}

		batches.push_back(std::move(batch));
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
			if (!cell.mountain.hasMountain) continue;

			const auto& mt = cell.mountain;
			float fx = static_cast<float>(x);
			float fz = static_cast<float>(z);
			float H = mt.Height();

			float SL = mt.slopeLeft.Total();
			float SR = mt.slopeRight.Total();
			float SF = mt.slopeFront.Total();
			float SB = mt.slopeBack.Total();

			glm::vec3 bot[4] = {
				{fx - SL,       0.0f, fz - SB      },
				{fx + 1.0f + SR, 0.0f, fz - SB      },
				{fx + 1.0f + SR, 0.0f, fz + 1.0f + SF},
				{fx - SL,       0.0f, fz + 1.0f + SF},
			};

			glm::vec3 top[4] = {
				{fx,       H, fz       },
				{fx + 1.0f, H, fz       },
				{fx + 1.0f, H, fz + 1.0f},
				{fx,       H, fz + 1.0f},
			};

			// Walls — partial culling based on neighbor height
			auto& wallBatch = findWallGroup(mt.texWall);

			// Left wall (-X)
			{
				float adjH = (x > 0 && m_grid[z][x - 1].mountain.hasMountain)
					? m_grid[z][x - 1].mountain.Height() : 0.0f;
				if (adjH < H)
				{
					float visibleH = H - adjH;
					glm::vec3 b0 = bot[3]; b0.y = adjH;
					glm::vec3 b1 = bot[0]; b1.y = adjH;
					addWallFace(wallBatch, top[3], top[0], b0, b1, { -1.0f, 0.0f, 0.0f }, visibleH);
				}
			}
			// Right wall (+X)
			{
				float adjH = (x < MAP_SIZE - 1 && m_grid[z][x + 1].mountain.hasMountain)
					? m_grid[z][x + 1].mountain.Height() : 0.0f;
				if (adjH < H)
				{
					float visibleH = H - adjH;
					glm::vec3 b0 = bot[1]; b0.y = adjH;
					glm::vec3 b1 = bot[2]; b1.y = adjH;
					addWallFace(wallBatch, top[1], top[2], b0, b1, { 1.0f, 0.0f, 0.0f }, visibleH);
				}
			}
			// Back wall (-Z)
			{
				float adjH = (z > 0 && m_grid[z - 1][x].mountain.hasMountain)
					? m_grid[z - 1][x].mountain.Height() : 0.0f;
				if (adjH < H)
				{
					float visibleH = H - adjH;
					glm::vec3 b0 = bot[0]; b0.y = adjH;
					glm::vec3 b1 = bot[1]; b1.y = adjH;
					addWallFace(wallBatch, top[0], top[1], b0, b1, { 0.0f, 0.0f, -1.0f }, visibleH);
				}
			}
			// Front wall (+Z)
			{
				float adjH = (z < MAP_SIZE - 1 && m_grid[z + 1][x].mountain.hasMountain)
					? m_grid[z + 1][x].mountain.Height() : 0.0f;
				if (adjH < H)
				{
					float visibleH = H - adjH;
					glm::vec3 b0 = bot[2]; b0.y = adjH;
					glm::vec3 b1 = bot[3]; b1.y = adjH;
					addWallFace(wallBatch, top[2], top[3], b0, b1, { 0.0f, 0.0f, 1.0f }, visibleH);
				}
			}

			// Top face
			auto& topBatch = findTopGroup(mt.texTop);
			// CCW from above: LB → LF → RF → RB
			addQuad(topBatch,
				top[0], top[3], top[2], top[1],
				{ 0.0f, 1.0f, 0.0f },
				{ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f });
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
void MapEditor::addQuad(
	MeshBatch& batch,
	glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
	glm::vec3 n,
	glm::vec2 uv0, glm::vec2 uv1, glm::vec2 uv2, glm::vec2 uv3)
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
	float texRepeatV)
{
	uint32_t base = static_cast<uint32_t>(batch.vertices.size());

	// UV mapping:
	// - U: along the edge direction (t0→t1 for top, b0→b1 for bottom)
	//   U = 0 at start, U = edge_length at end (texture repeats every 1.0)
	// - V: height, V = 0 at bottom, V = texRepeatV at top

	glm::vec3 topDir = t1 - t0;
	glm::vec3 botDir = b1 - b0;
	float topLen = glm::length(topDir);
	float botLen = glm::length(botDir);

	// For the top and bottom, we use the world-space distance along the wall
	// so the texture tiles every 1.0 world unit.
	float uTopStart = 0.0f;
	float uTopEnd = topLen;
	float uBotStart = 0.0f;
	float uBotEnd = botLen;

	// 4 vertices in triangle-strip order for a quad:
	// t0 (top-start) → t1 (top-end) → b0 (bot-start) → b1 (bot-end)
	// But for CCW winding from outside, we need:
	// For a wall facing -X (left), looking from -x:
	//   t0 is at front (+z), t1 is at back (-z) 
	//   From outside (-x view): y is up, z goes from gz+1 (front, near) to gz (back, far)
	//   CCW = start at top-left? hmm...

	// Let me use a generic approach: order vertices as t0, t1, b1, b0
	// This creates a quad where t0-t1 goes along the top and t0-b0 goes down.
	// For CCW from outside, the order depends on the normal direction.
	// With the convention that t0→t1 goes along the wall and b0 is below t0:
	// 
	// If the wall normal is +Z (front):
	//   Looking from +Z direction, +X is right, +Y is up
	//   t0 is at (left, H), t1 is at (right, H)
	//   CCW from outside (+Z): t0 → t1 → b1 → b0
	//   Normal (0,0,1)
	//   Cross product (t1-t0) × (b0-t0) should give normal direction
	//   t1-t0 = (1, 0, 0), b0-t0 = (0, -H, 0)
	//   Cross = (1,0,0) × (0,-H,0) = (0,0,-H) ... that's -Z, not +Z
	//   
	//   So to get +Z normal, we need: b0-t0 × t1-t0 = (0,-H,0) × (1,0,0) = (0,0,H) = +Z ✓
	//   So order is: t0, b0, b1, t1 → triangles (0,1,2), (2,3,0)

	// Let me use t0, b0, b1, t1 as the vertex order:
	// triangle 1: t0, b0, b1
	// triangle 2: b1, t1, t0

	// Actually, my addQuad uses these in CCW order: v0, v1, v2, v3
	// where triangles are (0,1,2) and (2,3,0)
	// So:
	// For a wall with normal pointing outward:
	// v0 = t0 (top, start of edge)
	// v1 = b0 (bottom, start of edge)
	// v2 = b1 (bottom, end of edge)
	// v3 = t1 (top, end of edge)
	// 
	// Check: (v1-v0) × (v2-v0) = (b0-t0) × (b1-t0)
	// For front wall (+Z), t0=(left,H), t1=(right,H), b0=(left,0), b1=(right,0):
	// (b0-t0) × (b1-t0) = (0,-H,0) × (1,0,0) = (0,0,H) ✓

	// Auto-correct winding: compute the face normal from (b0-t0) × (b1-t0)
	// and choose vertex order so both triangles face outward.
	glm::vec3 computedNormal = glm::cross(b0 - t0, b1 - t0);
	if (glm::dot(computedNormal, normal) >= 0.0f)
	{
		// Order A: t0, b0, b1, t1 — triangles (t0,b0,b1) and (b1,t1,t0)
		batch.vertices.push_back({ .position = t0, .normal = normal, .uv = {uTopStart, texRepeatV} });
		batch.vertices.push_back({ .position = b0, .normal = normal, .uv = {uBotStart, 0.0f} });
		batch.vertices.push_back({ .position = b1, .normal = normal, .uv = {uBotEnd, 0.0f} });
		batch.vertices.push_back({ .position = t1, .normal = normal, .uv = {uTopEnd, texRepeatV} });
	}
	else
	{
		// Order B: t0, t1, b1, b0 — triangles (t0,t1,b1) and (b1,b0,t0)
		batch.vertices.push_back({ .position = t0, .normal = normal, .uv = {uTopStart, texRepeatV} });
		batch.vertices.push_back({ .position = t1, .normal = normal, .uv = {uTopEnd, texRepeatV} });
		batch.vertices.push_back({ .position = b1, .normal = normal, .uv = {uBotEnd, 0.0f} });
		batch.vertices.push_back({ .position = b0, .normal = normal, .uv = {uBotStart, 0.0f} });
	}

	batch.indices.push_back(base + 0);
	batch.indices.push_back(base + 1);
	batch.indices.push_back(base + 2);
	batch.indices.push_back(base + 2);
	batch.indices.push_back(base + 3);
	batch.indices.push_back(base + 0);
}

//=============================================================================
void MapEditor::Update(scene::SceneManager& scene)
{
	// Rebuild geometry if dirty
	if (m_dirty)
	{
		RebuildGeometry(scene);
	}
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
	}

	if (m_state.activeTool == EditorTool::MountainBrush)
	{
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
		float totalHeight = static_cast<float>(m_state.mountainHeightBlocks) +
			static_cast<float>(m_state.mountainHeightPixels) / 16.0f;
		ImGui::Text("Total height: %.2f", totalHeight);

		float totalSlope = static_cast<float>(m_state.slopeBlocks) +
			static_cast<float>(m_state.slopePixels) / 16.0f;
		ImGui::Text("Total slope: %.2f", totalSlope);
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
		cell.floorTex = m_state.selectedFloorTex;
	}
	else if (m_state.activeTool == EditorTool::MountainBrush)
	{
		cell.mountain.hasMountain = true;
		cell.mountain.heightBlocks = static_cast<int16_t>(m_state.mountainHeightBlocks);
		cell.mountain.heightPixels = static_cast<int16_t>(m_state.mountainHeightPixels);
		cell.mountain.texWall = m_state.selectedWallTex;
		cell.mountain.texTop = m_state.selectedTopTex;

		cell.mountain.slopeLeft.blocks  = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
		cell.mountain.slopeLeft.pixels  = m_state.slopeLeft  ? static_cast<int16_t>(m_state.slopePixels) : 0;
		cell.mountain.slopeRight.blocks = m_state.slopeRight ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
		cell.mountain.slopeRight.pixels = m_state.slopeRight ? static_cast<int16_t>(m_state.slopePixels) : 0;
		cell.mountain.slopeFront.blocks = m_state.slopeFront ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
		cell.mountain.slopeFront.pixels = m_state.slopeFront ? static_cast<int16_t>(m_state.slopePixels) : 0;
		cell.mountain.slopeBack.blocks  = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopeBlocks) : 0;
		cell.mountain.slopeBack.pixels  = m_state.slopeBack  ? static_cast<int16_t>(m_state.slopePixels) : 0;
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
		cell.mountain.hasMountain = false;
	}

	m_dirty = true;
}

} //namespace map
