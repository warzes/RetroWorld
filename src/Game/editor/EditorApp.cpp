#include "stdafx.h"
#include "Editor.h"
#include <cmath>

namespace
{
	// VertexCP — one world-space vertex shared by 1..4 selected tiles
	struct VertexCP
	{
		glm::vec3 pos;
		int       refCount;                // how many tiles share this vertex
		int       refTX[4], refTY[4];      // tile coordinates for each ref
		int       refCorner[4];            // 0-3 floor, 4-7 ceiling per ref
	};

	constexpr int MAX_VERTEX_CPS = 2048;

	// Fill positions for all 10 Plane-mode control points (uses selection bounding box).
	// (selFX, selFZ) = position of tile (g_selTX, g_selTY), which is the tile CENTER.
	// Selection spans: X [selFX-0.5, selFX+selW-0.5], Z [selFZ-0.5, selFZ+selH-0.5]
	int GetCPPositions(glm::vec3* dst, int maxDst,
		float selFX, float selFZ, int selW, int selH,
		const tile::Tile& t, HeightEditMode mode) noexcept
	{
		float fh = t.floorHeight;
		float ch = t.ceilHeight;
		float avgFS = (t.slopeNW + t.slopeNE + t.slopeSE + t.slopeSW) * 0.25f;
		float avgCS = (t.ceilSlopeNW + t.ceilSlopeNE + t.ceilSlopeSE + t.ceilSlopeSW) * 0.25f;
		float cx = selFX + static_cast<float>(selW) * 0.5f - 0.5f;
		float cz = selFZ + static_cast<float>(selH) * 0.5f - 0.5f;
		float left   = selFX - 0.5f;
		float right  = selFX + static_cast<float>(selW) - 0.5f;
		float north  = selFZ - 0.5f;
		float south  = selFZ + static_cast<float>(selH) - 0.5f;

		if (mode == HeightEditMode::PLANE)
		{
			if (maxDst < 10) return 0;
			dst[0] = { cx, fh + avgFS, cz };                        // FloorCenter
			dst[1] = { cx, ch + avgCS, cz };                        // CeilCenter
			dst[2] = { cx, fh + avgFS, north };                     // FloorNorth
			dst[3] = { cx, fh + avgFS, south };                     // FloorSouth
			dst[4] = { left,  fh + avgFS, cz };                    // FloorWest
			dst[5] = { right, fh + avgFS, cz };                    // FloorEast
			dst[6] = { cx, ch + avgCS, north };                     // CeilNorth
			dst[7] = { cx, ch + avgCS, south };                     // CeilSouth
			dst[8] = { left,  ch + avgCS, cz };                    // CeilWest
			dst[9] = { right, ch + avgCS, cz };                    // CeilEast
			return 10;
		}
		else // VERTEX — single-tile markers (used when W=H=1)
		{
			if (maxDst < 8) return 0;
			dst[0] = { selFX - 0.5f, fh + t.slopeNW, selFZ - 0.5f };
			dst[1] = { selFX + 0.5f, fh + t.slopeNE, selFZ - 0.5f };
			dst[2] = { selFX + 0.5f, fh + t.slopeSE, selFZ + 0.5f };
			dst[3] = { selFX - 0.5f, fh + t.slopeSW, selFZ + 0.5f };
			dst[4] = { selFX - 0.5f, ch + t.ceilSlopeNW, selFZ - 0.5f };
			dst[5] = { selFX + 0.5f, ch + t.ceilSlopeNE, selFZ - 0.5f };
			dst[6] = { selFX + 0.5f, ch + t.ceilSlopeSE, selFZ + 0.5f };
			dst[7] = { selFX - 0.5f, ch + t.ceilSlopeSW, selFZ + 0.5f };
			return 8;
		}
	}

	// Collect all VERTEX-mode control points for the entire selection,
	// deduplicating by world-space position (epsilon 0.01).
	int CollectVertexCPs(VertexCP* dst, int maxDst)
	{
		int n = 0;
		for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
		{
			for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
			{
				if (!g_tileMap.InBounds(tx, ty)) continue;
				auto& t = g_tileMap.Get(tx, ty);
				if (t.spaceType != tile::TileSpaceType::SOLID) continue;
				float fx = static_cast<float>(tx);
				float fz = static_cast<float>(ty);
				float fh = t.floorHeight;
				float ch = t.ceilHeight;

				glm::vec3 corners[8] = {
					{fx - 0.5f, fh + t.slopeNW,     fz - 0.5f},
					{fx + 0.5f, fh + t.slopeNE,     fz - 0.5f},
					{fx + 0.5f, fh + t.slopeSE,     fz + 0.5f},
					{fx - 0.5f, fh + t.slopeSW,     fz + 0.5f},
					{fx - 0.5f, ch + t.ceilSlopeNW, fz - 0.5f},
					{fx + 0.5f, ch + t.ceilSlopeNE, fz - 0.5f},
					{fx + 0.5f, ch + t.ceilSlopeSE, fz + 0.5f},
					{fx - 0.5f, ch + t.ceilSlopeSW, fz + 0.5f},
				};

				for (int c = 0; c < 8; ++c)
				{
					int found = -1;
					for (int i = 0; i < n; ++i)
					{
						if (glm::distance(dst[i].pos, corners[c]) < 0.01f)
						{
							found = i;
							break;
						}
					}
					if (found >= 0)
					{
						auto& cp = dst[found];
						if (cp.refCount < 4)
						{
							int ri = cp.refCount;
							cp.refTX[ri] = tx;
							cp.refTY[ri] = ty;
							cp.refCorner[ri] = c;
							cp.refCount++;
						}
					}
					else
					{
						if (n >= maxDst) return n;
						auto& cp = dst[n];
						cp.pos = corners[c];
						cp.refCount = 1;
						cp.refTX[0] = tx;
						cp.refTY[0] = ty;
						cp.refCorner[0] = c;
						n++;
					}
				}
			}
		}
		return n;
	}

	void clampFloorCenter(float& fh, const float* slopes, const float* ceilSlopes, float ch) noexcept
	{
		float maxFH = ch - MIN_GAP;
		for (int i = 0; i < 4; ++i)
			maxFH = std::min(maxFH, ch + ceilSlopes[i] - slopes[i] - MIN_GAP);
		fh = std::min(fh, maxFH);
	}

	void clampCeilCenter(float& ch, const float* ceilSlopes, const float* slopes, float fh) noexcept
	{
		float minCH = fh + MIN_GAP;
		for (int i = 0; i < 4; ++i)
			minCH = std::max(minCH, fh + slopes[i] - ceilSlopes[i] + MIN_GAP);
		ch = std::max(ch, minCH);
	}

	void ApplyHeightStep(tile::Tile& t, int cpType, float delta) noexcept
	{
		float slopes[4] = { t.slopeNW, t.slopeNE, t.slopeSE, t.slopeSW };
		float ceilSlopes[4] = { t.ceilSlopeNW, t.ceilSlopeNE, t.ceilSlopeSE, t.ceilSlopeSW };

		switch (static_cast<CPType>(cpType))
		{
		case CPType::FloorCenter:
			t.floorHeight += delta;
			clampFloorCenter(t.floorHeight, slopes, ceilSlopes, t.ceilHeight);
			break;
		case CPType::CeilCenter:
			t.ceilHeight += delta;
			clampCeilCenter(t.ceilHeight, ceilSlopes, slopes, t.floorHeight);
			break;
		case CPType::FloorNorth:
			t.slopeNW += delta; t.slopeNE += delta;
			clampFloorVertex(t.slopeNW, t.floorHeight, t.ceilSlopeNW, t.ceilHeight);
			clampFloorVertex(t.slopeNE, t.floorHeight, t.ceilSlopeNE, t.ceilHeight);
			break;
		case CPType::CeilNorth:
			t.ceilSlopeNW += delta; t.ceilSlopeNE += delta;
			clampCeilVertex(t.ceilSlopeNW, t.ceilHeight, t.slopeNW, t.floorHeight);
			clampCeilVertex(t.ceilSlopeNE, t.ceilHeight, t.slopeNE, t.floorHeight);
			break;
		case CPType::FloorSouth:
			t.slopeSE += delta; t.slopeSW += delta;
			clampFloorVertex(t.slopeSE, t.floorHeight, t.ceilSlopeSE, t.ceilHeight);
			clampFloorVertex(t.slopeSW, t.floorHeight, t.ceilSlopeSW, t.ceilHeight);
			break;
		case CPType::CeilSouth:
			t.ceilSlopeSE += delta; t.ceilSlopeSW += delta;
			clampCeilVertex(t.ceilSlopeSE, t.ceilHeight, t.slopeSE, t.floorHeight);
			clampCeilVertex(t.ceilSlopeSW, t.ceilHeight, t.slopeSW, t.floorHeight);
			break;
		case CPType::FloorWest:
			t.slopeNW += delta; t.slopeSW += delta;
			clampFloorVertex(t.slopeNW, t.floorHeight, t.ceilSlopeNW, t.ceilHeight);
			clampFloorVertex(t.slopeSW, t.floorHeight, t.ceilSlopeSW, t.ceilHeight);
			break;
		case CPType::CeilWest:
			t.ceilSlopeNW += delta; t.ceilSlopeSW += delta;
			clampCeilVertex(t.ceilSlopeNW, t.ceilHeight, t.slopeNW, t.floorHeight);
			clampCeilVertex(t.ceilSlopeSW, t.ceilHeight, t.slopeSW, t.floorHeight);
			break;
		case CPType::FloorEast:
			t.slopeNE += delta; t.slopeSE += delta;
			clampFloorVertex(t.slopeNE, t.floorHeight, t.ceilSlopeNE, t.ceilHeight);
			clampFloorVertex(t.slopeSE, t.floorHeight, t.ceilSlopeSE, t.ceilHeight);
			break;
		case CPType::CeilEast:
			t.ceilSlopeNE += delta; t.ceilSlopeSE += delta;
			clampCeilVertex(t.ceilSlopeNE, t.ceilHeight, t.slopeNE, t.floorHeight);
			clampCeilVertex(t.ceilSlopeSE, t.ceilHeight, t.slopeSE, t.floorHeight);
			break;
		default: break;
		}
	}

	void ClampHeights(tile::Tile& t) noexcept
	{
		// Center-level safety: if gap too small, push only the ceiling up
		float avgFS = (t.slopeNW + t.slopeNE + t.slopeSE + t.slopeSW) * 0.25f;
		float avgCS = (t.ceilSlopeNW + t.ceilSlopeNE + t.ceilSlopeSE + t.ceilSlopeSW) * 0.25f;
		float diff = (t.ceilHeight + avgCS) - (t.floorHeight + avgFS);
		if (diff < MIN_GAP)
			t.ceilHeight += (MIN_GAP - diff);

		// Vertex-level safety: push ceiling up if gap is too small
		for (int c = 0; c < 4; ++c)
		{
			float* fSlope;
			float* cSlope;
			switch (c)
			{
				case 0: fSlope = &t.slopeNW; cSlope = &t.ceilSlopeNW; break;
				case 1: fSlope = &t.slopeNE; cSlope = &t.ceilSlopeNE; break;
				case 2: fSlope = &t.slopeSE; cSlope = &t.ceilSlopeSE; break;
				case 3: fSlope = &t.slopeSW; cSlope = &t.ceilSlopeSW; break;
				default: fSlope = &t.slopeNW; cSlope = &t.ceilSlopeNW; break;
			}

			float fY = t.floorHeight + *fSlope;
			float cY = t.ceilHeight + *cSlope;
			if (cY - fY < MIN_GAP)
				*cSlope += (MIN_GAP - (cY - fY));
		}
	}

	// Marker colors
	constexpr glm::vec4 COLOR_CENTER(1.0f, 1.0f, 0.0f, 1.0f);  // yellow for center markers
	constexpr glm::vec4 COLOR_EDGE(0.3f, 0.6f, 1.0f, 1.0f);    // blue for edge markers
	constexpr glm::vec4 COLOR_HOVER(1.0f, 1.0f, 1.0f, 1.0f);   // white
	constexpr glm::vec4 COLOR_CORNER(0.3f, 1.0f, 0.3f, 1.0f);  // green for floor corners
	constexpr glm::vec4 COLOR_CEIL_CORNER(1.0f, 0.6f, 0.1f, 1.0f);  // orange for ceiling corners
}

namespace
{
	const char* debugVert = R"(
#version 460 core
layout(location = 0) in vec3 a_position;
layout(location = 3) in vec4 a_color;
uniform mat4 u_viewProj;
out vec4 v_color;
void main() {
	v_color = a_color;
	gl_Position = u_viewProj * vec4(a_position, 1.0);
}
)";

	const char* debugFrag = R"(
#version 460 core
in vec4 v_color;
out vec4 o_color;
void main() {
	o_color = v_color;
}
)";

	const char* blinnPhongVert = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;
layout(location = 3) in vec4 a_color;

layout(std430, binding = 6) readonly buffer InstanceBuffer {
	mat4 models[];
} u_instanceData;

uniform bool  u_isInstanced;
uniform mat4  u_model;
uniform mat4  u_view;
uniform mat4  u_projection;
uniform mat3  u_normalMatrix;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_worldNormal;
layout(location = 2) out vec2 v_texcoord;
layout(location = 3) out vec4 v_color;

void main()
{
	mat4 model = u_isInstanced ? u_instanceData.models[gl_InstanceID] : u_model;
	mat3 normalMatrix = u_isInstanced ? transpose(inverse(mat3(model))) : u_normalMatrix;

	vec4 worldPos = model * vec4(a_position, 1.0);
	v_worldPos    = worldPos.xyz;
	v_worldNormal = normalize(normalMatrix * a_normal);
	v_texcoord    = a_texcoord;
	v_color       = a_color;

	gl_Position = u_projection * u_view * worldPos;
}
)";

	const char* blinnPhongFrag = R"(
#version 460 core

#define MAX_LIGHTS 16

struct LightData {
	vec4  positionOrDirection;
	vec3  color;
	float intensity;
	vec3  attenuation;
	float radius;
	vec3  spotDirection;
	float innerCutoff;
	float outerCutoff;
	int   type;
	bool  castShadow;
	float shadowBias;
	mat4  lightSpaceMatrix;
};

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec2 v_texcoord;
layout(location = 3) in vec4 v_color;

layout(std140, binding = 4) uniform LightBlock {
	int        u_lightCount;
	LightData  u_lights[MAX_LIGHTS];
};
uniform vec3       u_cameraPos;

uniform vec3  u_albedoColor;
uniform vec3  u_specularColor;
uniform vec3  u_ambientColor;
uniform float u_shininess;
uniform float u_opacity;

uniform bool       u_hasAlbedoMap;
uniform bool       u_hasNormalMap;
uniform bool       u_hasSpecularMap;
uniform bool       u_hasEmissiveMap;
layout(binding = 0) uniform sampler2D u_albedoMap;
layout(binding = 1) uniform sampler2D u_normalMap;
layout(binding = 2) uniform sampler2D u_specularMap;
layout(binding = 3) uniform sampler2D u_emissiveMap;

uniform bool       u_receiveShadow;
uniform sampler2D  u_shadowMap;
uniform float      u_shadowMapSize;
uniform samplerCube u_pointShadowMap;
uniform float       u_pointShadowMapSize;

layout(location = 0) out vec4 o_color;

float ShadowCalculation(vec4 lightSpacePos, float bias)
{
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	float currentDepth = projCoords.z - bias;
	if (currentDepth > 1.0) return 1.0;
	float shadow = 0.0;
	float texelSize = 1.0 / u_shadowMapSize;
	for (int x = -1; x <= 1; ++x)
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(u_shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth < pcfDepth ? 1.0 : 0.0;
		}
	return shadow / 9.0;
}

float PointShadowCalculation(vec3 fragToLight, float bias, float farPlane)
{
	float closestDepth = texture(u_pointShadowMap, normalize(fragToLight)).r;
	float currentDepth = length(fragToLight) / farPlane;
	return (currentDepth - bias) < closestDepth ? 1.0 : 0.0;
}

vec3 CalcDirectionalLight(LightData light, vec3 N, vec3 V, vec3 albedo, float shininess)
{
	vec3 L = normalize(-light.spotDirection);
	vec3 H = normalize(L + V);
	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);
	float shadow = 1.0;
	if (light.castShadow && u_receiveShadow)
	{
		vec4 lightSpacePos = light.lightSpaceMatrix * vec4(v_worldPos, 1.0);
		shadow = ShadowCalculation(lightSpacePos, light.shadowBias);
	}
	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * vec3(0.1) * pow(NdotH, shininess);
	return (albedo * 0.05 + (diffuse + spec) * shadow) * light.intensity;
}

void main()
{
	vec3 albedo = u_albedoColor * v_color.rgb;
	vec3 normal = normalize(v_worldNormal);
	if (u_hasAlbedoMap) albedo *= texture(u_albedoMap, v_texcoord).rgb;
	vec3 V = normalize(u_cameraPos - v_worldPos);
	vec3 result = vec3(0.0);
	for (int i = 0; i < u_lightCount && i < MAX_LIGHTS; ++i)
	{
		LightData light = u_lights[i];
		if (light.type == 0)
			result += CalcDirectionalLight(light, normal, V, albedo, u_shininess);
	}
	o_color = vec4(result, u_opacity);
}
)";

	const char* shadowDepthVert = R"(
#version 460 core
layout(location = 0) in vec3 a_position;
layout(std430, binding = 6) readonly buffer InstanceBuffer {
	mat4 models[];
} u_instanceData;
uniform bool  u_isInstanced;
uniform mat4  u_lightVP;
uniform mat4  u_model;
void main()
{
	mat4 model = u_isInstanced ? u_instanceData.models[gl_InstanceID] : u_model;
	gl_Position = u_lightVP * model * vec4(a_position, 1.0);
}
)";

	const char* shadowDepthFrag = R"(#version 460 core
void main() {}
)";
}

// ---- GameInit ----
bool GameInit()
{
	// Shaders
	{
		gpu::program::GraphicsProgramCreateInfo ci{ .name = "BlinnPhong",
			.vertexShaderCode = blinnPhongVert, .fragmentShaderCode = blinnPhongFrag };
		g_program = gpu::program::CreateShaderProgram(ci);
		if (!gpu::program::IsValid(g_program)) { core::Error("failed BlinnPhong"); return false; }
	}
	{
		gpu::program::GraphicsProgramCreateInfo ci{ .name = "ShadowDepth",
			.vertexShaderCode = shadowDepthVert, .fragmentShaderCode = shadowDepthFrag };
		g_depthShader = gpu::program::CreateShaderProgram(ci);
		if (!gpu::program::IsValid(g_depthShader)) { core::Error("failed ShadowDepth"); return false; }
	}
	{
		gpu::program::GraphicsProgramCreateInfo ci{ .name = "Debug",
			.vertexShaderCode = debugVert, .fragmentShaderCode = debugFrag };
		g_debugProgram = gpu::program::CreateShaderProgram(ci);
		if (!gpu::program::IsValid(g_debugProgram)) { core::Error("failed Debug"); return false; }
	}

	// Tile map
	g_tileMap.Resize(g_mapWidth, g_mapHeight);
	g_tileMap.GenerateRandom(++g_genSeed);

	// Atlas texture (repeating format for proper wall tiling)
	g_atlasTex = tile::CreateRepeatingWallAtlas(64, 8);
	gpu::texture::SamplerState ss{};
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
	ss.addressModeV = gpu::AddressMode::Repeat;
	g_atlasSampler = gpu::texture::CreateSampler(ss);

	// Material
	g_tileMaterial.albedoMap  = g_atlasTex;
	g_tileMaterial.sampler    = g_atlasSampler;
	g_tileMaterial.albedoColor   = { 1, 1, 1 };
	g_tileMaterial.specularColor = { 0.05f, 0.05f, 0.05f };
	g_tileMaterial.ambientColor  = { 0.08f, 0.08f, 0.08f };
	g_tileMaterial.shininess = 8.0f;

	// SceneManager
	g_scene = std::make_unique<scene::SceneManager>();
	auto& root = *g_scene->root;

	// Camera
	auto& camNode = root.AddChild<scene::CameraNode>("camera");
	camNode.externalCamera = &g_camera;
	camNode.aspectRatio = window::GetAspectRatio();
	camNode.farPlane = 200.0f;
	g_scene->SetActiveCamera(camNode);

	// Sun
	auto& sun = root.AddChild<scene::LightNode>("sun");
	sun.lightType = scene::LightNode::LightType::Directional;
	sun.color = { 1.0f, 0.95f, 0.85f };
	sun.intensity = 1.5f;
	sun.castShadow = true;
	sun.shadowSettings.resolution = 2048;
	sun.shadowSettings.orthoSize = 40.0f;
	sun.shadowSettings.cascadeDistance[0] = -1.0f;
	sun.shadowSettings.cascadeDistance[1] = -1.0f;
	sun.shadowSettings.cascadeDistance[2] = -1.0f;
	sun.shadowSettings.cascadeDistance[3] = -1.0f;
	sun.transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(1, 0, 1));

	// Tile model node
	auto& tileNode = root.AddChild<scene::ModelNode>("tiles");
	g_tileModelNode = &tileNode;
	tileNode.castShadow    = true;
	tileNode.receiveShadow = true;
	tileNode.material = std::make_shared<gr::Material>(g_tileMaterial);

	RebuildTileMesh();

	g_camera = gr::Camera(
		glm::vec3(0.0f, 5.0f, 5.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0, 1, 0));

	g_scene->enableShadows    = true;
	g_scene->enableInstancing = false;
	return true;
}

// ---- GameClose ----
void GameClose()
{
	g_tileMesh.Close();
	g_pointDepthShader.reset();
	g_depthShader.reset();
	g_program.reset();
	g_debugProgram.reset();
	g_atlasTex.reset();
	g_atlasSampler.reset();
	g_scene.reset();
	g_mouseLook.Reset();
}

// ---- GameUpdate ----
void GameUpdate()
{
	const float speed = 10.0f * app::GetDeltaTime();
	bool wantCaptureMouse = ImGui::GetIO().WantCaptureMouse;

	if (input::IsKeyDown(KeyboardType::KEY_W)) g_camera.Move(gr::Movement::Forward,  speed);
	if (input::IsKeyDown(KeyboardType::KEY_S)) g_camera.Move(gr::Movement::Backward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_A)) g_camera.Move(gr::Movement::Left,     speed);
	if (input::IsKeyDown(KeyboardType::KEY_D)) g_camera.Move(gr::Movement::Right,    speed);
	if (input::IsKeyDown(KeyboardType::KEY_Q)) g_camera.Move(gr::Movement::Down,     speed);
	if (input::IsKeyDown(KeyboardType::KEY_E)) g_camera.Move(gr::Movement::Up,       speed);

	if (!wantCaptureMouse && input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
		g_mouseLook.OnRightDown();
	else
		g_mouseLook.OnRightUp();
	g_mouseLook.Update(g_camera);

	// Mode switching
	if (input::IsKeyDown(KeyboardType::KEY_1)) { g_editMode = EditMode::TILE;   g_selCorner = -1; }
	if (input::IsKeyDown(KeyboardType::KEY_2)) { g_editMode = EditMode::FACE;   g_selCorner = -1; }
	if (input::IsKeyDown(KeyboardType::KEY_3)) { g_editMode = EditMode::VERTEX; g_selFace = tile::FaceDir::COUNT; }

	// V key toggles height edit sub-mode in TILE mode
	if (input::IsKeyDown(KeyboardType::KEY_V) && g_editMode == EditMode::TILE)
	{
		g_heightEditMode = (g_heightEditMode == HeightEditMode::PLANE)
			? HeightEditMode::VERTEX : HeightEditMode::PLANE;
	}

	// Regenerate
	if (input::IsKeyDown(KeyboardType::KEY_R))
	{
		g_tileMap.GenerateRandom(++g_genSeed);
		g_selTX = g_selTY = g_selCorner = -1;
		g_selW = 1; g_selH = 1;
		g_selFace = tile::FaceDir::COUNT;

		g_dirtyMesh = true;
	}

	// Delete key — remove all selected tiles
	if (input::IsKeyDown(KeyboardType::KEY_DELETE) && g_selTX >= 0)
	{
		for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
			for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
			{
				if (!g_tileMap.InBounds(tx, ty)) continue;
				auto& tt = g_tileMap.Get(tx, ty);
				tt.spaceType   = tile::TileSpaceType::EMPTY;
				tt.renderSolid = false;
			}
		g_dirtyMesh = true;
	}

	// Enter key — carve (place) empty tiles in selection
	if ((input::IsKeyDown(KeyboardType::KEY_ENTER) || input::IsKeyDown(KeyboardType::KEY_KP_ENTER)) && g_selTX >= 0)
	{
		for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
			for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
			{
				if (!g_tileMap.InBounds(tx, ty)) continue;
				auto& tt = g_tileMap.Get(tx, ty);
				if (tt.spaceType == tile::TileSpaceType::SOLID) continue;
				tt.spaceType   = tile::TileSpaceType::SOLID;
				tt.renderSolid = true;
				tt.wallTex     = static_cast<uint8_t>(g_brushWallTex);
				tt.floorTex    = static_cast<uint8_t>(g_brushFloorTex);
				tt.ceilTex     = static_cast<uint8_t>(g_brushCeilTex);
			}
		g_dirtyMesh = true;
	}

	static bool prevLMB = false;
	bool lmb = input::IsMouseDown(MouseType::MOUSE_BUTTON_LEFT);
	bool lmbPressed = lmb && !prevLMB;

	if (!wantCaptureMouse)
	{
		// Pre-compute ray and mouse state for all picking/hover ops
		glm::vec3 rayDir{};
		glm::vec3 camPos{};
		glm::vec2 mousePos{};
		if (g_scene->activeCamera)
		{
			mousePos = { input::GetMousePosition().x, input::GetMousePosition().y };
			auto vp = g_scene->activeCamera->GetViewProjectionMatrix();
			float ww = static_cast<float>(window::GetWidth());
			float wh = static_cast<float>(window::GetHeight());
			rayDir = tile::ScreenToRay(vp, mousePos.x, mousePos.y, ww, wh);
			camPos = g_scene->activeCamera->GetPosition();
		}

		// --- CP hover detection ---
		g_hoverCPIdx = -1;
		if (g_editMode == EditMode::TILE && g_selTX >= 0 && g_tileMap.InBounds(g_selTX, g_selTY) && g_scene->activeCamera)
		{
			auto& t = g_tileMap.Get(g_selTX, g_selTY);
			if (t.spaceType == tile::TileSpaceType::SOLID)
			{
				float fx = static_cast<float>(g_selTX);
				float fz = static_cast<float>(g_selTY);

				if (g_heightEditMode == HeightEditMode::PLANE)
				{
				// Plane mode: CPs at selection bounding box
				glm::vec3 cps[10];
				int n = GetCPPositions(cps, 10, fx, fz, g_selW, g_selH, t, HeightEditMode::PLANE);
				float bestDist = 0.28f;
				for (int i = 0; i < n; ++i)
				{
					glm::vec3 d = cps[i] - camPos;
					float td = glm::dot(d, rayDir);
					if (td <= 0) continue;
					float dist = glm::distance(cps[i], camPos + rayDir * td);
					if (dist < bestDist) { bestDist = dist; g_hoverCPIdx = i; }
				}
			}
			else
			{
				// Vertex mode: collect CPs from all solid tiles in selection
				VertexCP vcps[MAX_VERTEX_CPS];
				int vn = CollectVertexCPs(vcps, MAX_VERTEX_CPS);
				float bestDist = 0.28f;
				for (int i = 0; i < vn; ++i)
				{
					glm::vec3 d = vcps[i].pos - camPos;
					float td = glm::dot(d, rayDir);
					if (td <= 0) continue;
					float dist = glm::distance(vcps[i].pos, camPos + rayDir * td);
					if (dist < bestDist) { bestDist = dist; g_hoverCPIdx = i; }
				}
			}
		}
	}

	// --- Left click ---
		if (lmbPressed && g_scene->activeCamera)
		{
			bool cpPicked = false;

			if (g_hoverCPIdx >= 0)
			{
				if (g_heightEditMode == HeightEditMode::VERTEX)
				{
					VertexCP vcps[MAX_VERTEX_CPS];
					int vn = CollectVertexCPs(vcps, MAX_VERTEX_CPS);
					if (g_hoverCPIdx < vn)
					{
						const auto& vcp = vcps[g_hoverCPIdx];
						g_dragVtxRefCount = vcp.refCount;
						for (int ri = 0; ri < vcp.refCount; ++ri)
						{
							g_dragVtxTX[ri] = vcp.refTX[ri];
							g_dragVtxTY[ri] = vcp.refTY[ri];
							g_dragVtxCorner[ri] = vcp.refCorner[ri];
							auto& t = g_tileMap.Get(vcp.refTX[ri], vcp.refTY[ri]);
							int corner = vcp.refCorner[ri] & 3;
							if (vcp.refCorner[ri] < 4)
								g_dragVtxInitSlope[ri] = (&t.slopeNW)[corner];
							else
								g_dragVtxInitSlope[ri] = (&t.ceilSlopeNW)[corner];
						}
						// Save first tile slopes for reference
						{
							auto& t0 = g_tileMap.Get(vcp.refTX[0], vcp.refTY[0]);
							g_dragSlopes[0] = t0.slopeNW;
							g_dragSlopes[1] = t0.slopeNE;
							g_dragSlopes[2] = t0.slopeSE;
							g_dragSlopes[3] = t0.slopeSW;
							g_dragCeilSlopes[0] = t0.ceilSlopeNW;
							g_dragCeilSlopes[1] = t0.ceilSlopeNE;
							g_dragCeilSlopes[2] = t0.ceilSlopeSE;
							g_dragCeilSlopes[3] = t0.ceilSlopeSW;
						}
						g_draggingCP = true;
						g_dragCPType = vcp.refCorner[0];
						g_dragStartMouseY = mousePos.y;
						g_lastAppliedDy = 0;
						cpPicked = true;
					}
				}
				else // PLANE
				{
					auto& t = g_tileMap.Get(g_selTX, g_selTY);
					g_draggingCP = true;
					g_dragCPType = g_hoverCPIdx;
					g_dragStartMouseY = mousePos.y;
					g_lastAppliedDy = 0;
					g_dragSlopes[0] = t.slopeNW;
					g_dragSlopes[1] = t.slopeNE;
					g_dragSlopes[2] = t.slopeSE;
					g_dragSlopes[3] = t.slopeSW;
					g_dragCeilSlopes[0] = t.ceilSlopeNW;
					g_dragCeilSlopes[1] = t.ceilSlopeNE;
					g_dragCeilSlopes[2] = t.ceilSlopeSE;
					g_dragCeilSlopes[3] = t.ceilSlopeSW;
					cpPicked = true;
				}
			}

			if (!cpPicked)
			{
				PickTile(camPos, rayDir);
				// Start tile-rect drag
				if (g_selTX >= 0)
				{
					g_draggingSel = true;
					g_dragStartTX = g_selTX;
					g_dragStartTY = g_selTY;
					g_selW = 1; g_selH = 1;
				}
			}
		}

		// --- Tile selection drag (expand rectangle) ---
		if (g_draggingSel && lmb && g_scene->activeCamera)
		{
			int newX = -1, newY = -1;
			tile::HitInfo hit;
			if (g_tileMeshCPU.RayIntersect(camPos, rayDir, hit) && g_tileMap.InBounds(hit.tileX, hit.tileY))
			{
				newX = hit.tileX;
				newY = hit.tileY;
			}
			else if (fabsf(rayDir.y) > 1e-6f)
			{
				float t = -camPos.y / rayDir.y;
				if (t > 0)
				{
					glm::vec3 hp = camPos + rayDir * t;
					int tx = static_cast<int>(floor(hp.x + 0.5f));
					int ty = static_cast<int>(floor(hp.z + 0.5f));
					if (g_tileMap.InBounds(tx, ty))
					{
						newX = tx;
						newY = ty;
					}
				}
			}
			if (newX >= 0)
			{
				g_selTX = (std::min)(g_dragStartTX, newX);
				g_selTY = (std::min)(g_dragStartTY, newY);
				g_selW = abs(g_dragStartTX - newX) + 1;
				g_selH = abs(g_dragStartTY - newY) + 1;
				g_dirtyMesh = true;
			}
		}
		if (!lmb && g_draggingSel)
			g_draggingSel = false;

		// --- Face hover ---
		{
			tile::HitInfo hit;
			if (g_scene->activeCamera && g_tileMeshCPU.RayIntersect(camPos, rayDir, hit))
			{
				g_hoverTX = hit.tileX;
				g_hoverTY = hit.tileY;
				g_hoverFace = hit.face;
			}
			else
			{
				g_hoverTX = g_hoverTY = -1;
				g_hoverFace = tile::FaceDir::COUNT;
			}

			if (g_hoverTX != g_prevHoverTX || g_hoverTY != g_prevHoverTY ||
				g_hoverFace != g_prevHoverFace)
			{
				g_dirtyMesh = true;
				g_prevHoverTX = g_hoverTX;
				g_prevHoverTY = g_hoverTY;
				g_prevHoverFace = g_hoverFace;
			}
		}

		// --- CP dragging (step-based) ---
		if (g_editMode == EditMode::TILE && g_draggingCP && lmb && g_selTX >= 0)
		{
			float totalDy = -(mousePos.y - g_dragStartMouseY) * 0.02f;

			// Snap to nearest multiple of step
			int numSteps;
			if (totalDy >= 0)
				numSteps = static_cast<int>(totalDy / g_heightStep);
			else
				numSteps = -static_cast<int>(-totalDy / g_heightStep);
			float snappedDy = static_cast<float>(numSteps) * g_heightStep;

			float delta = snappedDy - g_lastAppliedDy;
			if (fabsf(delta) >= 1e-5f)
			{
				auto applyToAll = [&](int cpType, float mod)
				{
					for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
					{
						for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
						{
							if (!g_tileMap.InBounds(tx, ty)) continue;
							auto& tile = g_tileMap.Get(tx, ty);
							if (tile.spaceType != tile::TileSpaceType::SOLID) continue;
							ApplyHeightStep(tile, cpType, delta * mod);
							ClampHeights(tile);
						}
					}
				};

				if (g_heightEditMode == HeightEditMode::PLANE)
				{
					if (g_dragCPType <= static_cast<int>(CPType::CeilCenter))
					{
						// Center edits: uniform for all tiles
						applyToAll(g_dragCPType, 1.0f);
					}
					else
					{
						// Edge edits: adjust primary + secondary pairs with interpolation
						// to maintain C0 continuity across tile boundaries
						for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
						{
							for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
							{
								if (!g_tileMap.InBounds(tx, ty)) continue;
								auto& tile = g_tileMap.Get(tx, ty);
								if (tile.spaceType != tile::TileSpaceType::SOLID) continue;

								int cy = ty - g_selTY;
								int cx = tx - g_selTX;
								float pMod = 0.0f; // primary pair (near dragged edge)
								float sMod = 0.0f; // secondary pair (far from dragged edge)

								switch (static_cast<CPType>(g_dragCPType))
								{
								case CPType::FloorNorth:
									pMod = 1.0f - static_cast<float>(cy) / g_selH;
									sMod = 1.0f - static_cast<float>(cy + 1) / g_selH;
									tile.slopeNW += delta * pMod;
									tile.slopeNE += delta * pMod;
									tile.slopeSE += delta * sMod;
									tile.slopeSW += delta * sMod;
									clampFloorVertex(tile.slopeNW, tile.floorHeight, tile.ceilSlopeNW, tile.ceilHeight);
									clampFloorVertex(tile.slopeNE, tile.floorHeight, tile.ceilSlopeNE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSE, tile.floorHeight, tile.ceilSlopeSE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSW, tile.floorHeight, tile.ceilSlopeSW, tile.ceilHeight);
									break;
								case CPType::CeilNorth:
									pMod = 1.0f - static_cast<float>(cy) / g_selH;
									sMod = 1.0f - static_cast<float>(cy + 1) / g_selH;
									tile.ceilSlopeNW += delta * pMod;
									tile.ceilSlopeNE += delta * pMod;
									tile.ceilSlopeSE += delta * sMod;
									tile.ceilSlopeSW += delta * sMod;
									clampCeilVertex(tile.ceilSlopeNW, tile.ceilHeight, tile.slopeNW, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeNE, tile.ceilHeight, tile.slopeNE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSE, tile.ceilHeight, tile.slopeSE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSW, tile.ceilHeight, tile.slopeSW, tile.floorHeight);
									break;
								case CPType::FloorSouth:
									pMod = static_cast<float>(cy + 1) / g_selH;
									sMod = static_cast<float>(cy) / g_selH;
									tile.slopeSE += delta * pMod;
									tile.slopeSW += delta * pMod;
									tile.slopeNW += delta * sMod;
									tile.slopeNE += delta * sMod;
									clampFloorVertex(tile.slopeNW, tile.floorHeight, tile.ceilSlopeNW, tile.ceilHeight);
									clampFloorVertex(tile.slopeNE, tile.floorHeight, tile.ceilSlopeNE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSE, tile.floorHeight, tile.ceilSlopeSE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSW, tile.floorHeight, tile.ceilSlopeSW, tile.ceilHeight);
									break;
								case CPType::CeilSouth:
									pMod = static_cast<float>(cy + 1) / g_selH;
									sMod = static_cast<float>(cy) / g_selH;
									tile.ceilSlopeSE += delta * pMod;
									tile.ceilSlopeSW += delta * pMod;
									tile.ceilSlopeNW += delta * sMod;
									tile.ceilSlopeNE += delta * sMod;
									clampCeilVertex(tile.ceilSlopeNW, tile.ceilHeight, tile.slopeNW, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeNE, tile.ceilHeight, tile.slopeNE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSE, tile.ceilHeight, tile.slopeSE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSW, tile.ceilHeight, tile.slopeSW, tile.floorHeight);
									break;
								case CPType::FloorWest:
									pMod = 1.0f - static_cast<float>(cx) / g_selW;
									sMod = 1.0f - static_cast<float>(cx + 1) / g_selW;
									tile.slopeNW += delta * pMod;
									tile.slopeSW += delta * pMod;
									tile.slopeNE += delta * sMod;
									tile.slopeSE += delta * sMod;
									clampFloorVertex(tile.slopeNW, tile.floorHeight, tile.ceilSlopeNW, tile.ceilHeight);
									clampFloorVertex(tile.slopeNE, tile.floorHeight, tile.ceilSlopeNE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSE, tile.floorHeight, tile.ceilSlopeSE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSW, tile.floorHeight, tile.ceilSlopeSW, tile.ceilHeight);
									break;
								case CPType::CeilWest:
									pMod = 1.0f - static_cast<float>(cx) / g_selW;
									sMod = 1.0f - static_cast<float>(cx + 1) / g_selW;
									tile.ceilSlopeNW += delta * pMod;
									tile.ceilSlopeSW += delta * pMod;
									tile.ceilSlopeNE += delta * sMod;
									tile.ceilSlopeSE += delta * sMod;
									clampCeilVertex(tile.ceilSlopeNW, tile.ceilHeight, tile.slopeNW, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeNE, tile.ceilHeight, tile.slopeNE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSE, tile.ceilHeight, tile.slopeSE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSW, tile.ceilHeight, tile.slopeSW, tile.floorHeight);
									break;
								case CPType::FloorEast:
									pMod = static_cast<float>(cx + 1) / g_selW;
									sMod = static_cast<float>(cx) / g_selW;
									tile.slopeNE += delta * pMod;
									tile.slopeSE += delta * pMod;
									tile.slopeNW += delta * sMod;
									tile.slopeSW += delta * sMod;
									clampFloorVertex(tile.slopeNW, tile.floorHeight, tile.ceilSlopeNW, tile.ceilHeight);
									clampFloorVertex(tile.slopeNE, tile.floorHeight, tile.ceilSlopeNE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSE, tile.floorHeight, tile.ceilSlopeSE, tile.ceilHeight);
									clampFloorVertex(tile.slopeSW, tile.floorHeight, tile.ceilSlopeSW, tile.ceilHeight);
									break;
								case CPType::CeilEast:
									pMod = static_cast<float>(cx + 1) / g_selW;
									sMod = static_cast<float>(cx) / g_selW;
									tile.ceilSlopeNE += delta * pMod;
									tile.ceilSlopeSE += delta * pMod;
									tile.ceilSlopeNW += delta * sMod;
									tile.ceilSlopeSW += delta * sMod;
									clampCeilVertex(tile.ceilSlopeNW, tile.ceilHeight, tile.slopeNW, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeNE, tile.ceilHeight, tile.slopeNE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSE, tile.ceilHeight, tile.slopeSE, tile.floorHeight);
									clampCeilVertex(tile.ceilSlopeSW, tile.ceilHeight, tile.slopeSW, tile.floorHeight);
									break;
								default: break;
								}
								ClampHeights(tile);
							}
						}
					}
				}
				else
				{
					// VERTEX mode — apply delta to all tiles sharing this vertex
					for (int ri = 0; ri < g_dragVtxRefCount; ++ri)
					{
						auto& tile = g_tileMap.Get(g_dragVtxTX[ri], g_dragVtxTY[ri]);
						int corner = g_dragVtxCorner[ri] & 3;
						if (g_dragVtxCorner[ri] < 4) // floor corner
						{
							float* dst[4] = { &tile.slopeNW, &tile.slopeNE, &tile.slopeSE, &tile.slopeSW };
							float* cSlope[4] = { &tile.ceilSlopeNW, &tile.ceilSlopeNE, &tile.ceilSlopeSE, &tile.ceilSlopeSW };
							*dst[corner] = g_dragVtxInitSlope[ri] + snappedDy;
							clampFloorVertex(*dst[corner], tile.floorHeight, *cSlope[corner], tile.ceilHeight);
						}
						else // ceiling corner
						{
							float* dst[4] = { &tile.ceilSlopeNW, &tile.ceilSlopeNE, &tile.ceilSlopeSE, &tile.ceilSlopeSW };
							float* fSlope[4] = { &tile.slopeNW, &tile.slopeNE, &tile.slopeSE, &tile.slopeSW };
							*dst[corner] = g_dragVtxInitSlope[ri] + snappedDy;
							clampCeilVertex(*dst[corner], tile.ceilHeight, *fSlope[corner], tile.floorHeight);
						}
						ClampHeights(tile);
					}
				}
				g_lastAppliedDy = snappedDy;
				g_dirtyMesh = true;
			}
		}

		// Cancel CP drag on LMB release
		if (!lmb && g_draggingCP)
		{
			g_draggingCP = false;
			g_dragVtxRefCount = 0;
		}

		// Scroll to adjust height in VERTEX mode (slope adjustment)
		{
			static float scrollAccum = 0.0f;
			float md = input::GetMouseDelta();
			if (g_editMode == EditMode::VERTEX && g_selTX >= 0 && g_selCorner >= 0 && fabsf(md) > 0.5f)
			{
				scrollAccum += md * 0.01f;
				if (fabsf(scrollAccum) > 0.05f)
				{
					float delta = (scrollAccum > 0 ? 0.1f : -0.1f);
					auto& t = g_tileMap.Get(g_selTX, g_selTY);
					float* fSlope = nullptr;
					float* cSlope = nullptr;
					switch (g_selCorner)
					{
						case 0: fSlope = &t.slopeNW; cSlope = &t.ceilSlopeNW; break;
						case 1: fSlope = &t.slopeNE; cSlope = &t.ceilSlopeNE; break;
						case 2: fSlope = &t.slopeSE; cSlope = &t.ceilSlopeSE; break;
						case 3: fSlope = &t.slopeSW; cSlope = &t.ceilSlopeSW; break;
					}
					if (fSlope && cSlope)
					{
						*fSlope += delta;
						clampFloorVertex(*fSlope, t.floorHeight, *cSlope, t.ceilHeight);
					}
					ClampHeights(t);
					g_dirtyMesh = true;
					scrollAccum = 0.0f;
				}
			}
			else scrollAccum = 0.0f;
		}
	}
	prevLMB = lmb;

	if (g_scene->activeCamera)
		g_scene->activeCamera->aspectRatio = window::GetAspectRatio();

	if (g_dirtyMesh)
		RebuildTileMesh();

	g_scene->Update();
}

// ---- GameFixedUpdate ----
void GameFixedUpdate() {}

// ---- DrawDebugOverlay ----
void DrawDebugOverlay()
{
	if (!g_debugProgram || !g_scene->activeCamera) return;
	if (g_selTX < 0 || !g_tileMap.InBounds(g_selTX, g_selTY)) return;

	// Camera basis vectors for billboarding
	glm::mat4 camView = g_scene->activeCamera->GetViewMatrix();
	glm::vec3 camRight = glm::normalize(glm::vec3(camView[0][0], camView[1][0], camView[2][0]));
	glm::vec3 camUp    = glm::normalize(glm::vec3(camView[0][1], camView[1][1], camView[2][1]));
	const float markerHalf = 0.13f;

	std::vector<gr::MeshVertex> lines;
	std::vector<gr::MeshVertex> tris;
	glm::vec4 wireColor(0.8f, 0.8f, 0.8f, 0.6f);

	auto addLine = [&](glm::vec3 a, glm::vec3 b, glm::vec4 c)
	{
		lines.push_back({ a, {}, {}, c });
		lines.push_back({ b, {}, {}, c });
	};

	auto addRect = [&](const glm::vec3& center, const glm::vec4& color)
	{
		glm::vec3 r = camRight * markerHalf;
		glm::vec3 u = camUp * markerHalf;
		glm::vec3 p0 = center - r - u;
		glm::vec3 p1 = center + r - u;
		glm::vec3 p2 = center + r + u;
		glm::vec3 p3 = center - r + u;
		tris.push_back({ p0, {}, {}, color });
		tris.push_back({ p1, {}, {}, color });
		tris.push_back({ p2, {}, {}, color });
		tris.push_back({ p0, {}, {}, color });
		tris.push_back({ p2, {}, {}, color });
		tris.push_back({ p3, {}, {}, color });
	};

	// Wireframe and CP markers
	if (g_heightEditMode == HeightEditMode::PLANE)
	{
		// Bounding-box wireframe for the whole selection
		auto& t = g_tileMap.Get(g_selTX, g_selTY);
		float fx = static_cast<float>(g_selTX);
		float fz = static_cast<float>(g_selTY);
		float left  = fx - 0.5f;
		float right = fx + static_cast<float>(g_selW) - 0.5f;
		float nz    = fz - 0.5f;
		float sz    = fz + static_cast<float>(g_selH) - 0.5f;

		float avgFS = (t.slopeNW + t.slopeNE + t.slopeSE + t.slopeSW) * 0.25f;
		float avgCS = (t.ceilSlopeNW + t.ceilSlopeNE + t.ceilSlopeSE + t.ceilSlopeSW) * 0.25f;
		float fh = t.floorHeight + avgFS;
		float ch = t.ceilHeight  + avgCS;

		// 8 corners of the selection bounding box
		glm::vec3 btm[4] = {
			{ left,  fh, nz },
			{ right, fh, nz },
			{ right, fh, sz },
			{ left,  fh, sz },
		};
		glm::vec3 top[4] = {
			{ left,  ch, nz },
			{ right, ch, nz },
			{ right, ch, sz },
			{ left,  ch, sz },
		};

		for (int i = 0; i < 4; ++i)
			addLine(btm[i], btm[(i + 1) % 4], wireColor);
		for (int i = 0; i < 4; ++i)
			addLine(top[i], top[(i + 1) % 4], wireColor);
		for (int i = 0; i < 4; ++i)
			addLine(btm[i], top[i], wireColor);

		// PLANE mode CPs at selection bounding-box
		glm::vec3 cps[10];
		int n = GetCPPositions(cps, 10, fx, fz, g_selW, g_selH, t, HeightEditMode::PLANE);
		for (int i = 0; i < n; ++i)
		{
			bool isCenter = (i <= static_cast<int>(CPType::CeilCenter));
			bool hovered = (g_hoverCPIdx == i);
			glm::vec4 col = hovered ? COLOR_HOVER : (isCenter ? COLOR_CENTER : COLOR_EDGE);
			addRect(cps[i], col);
		}
	}
	else // VERTEX mode — per-tile wireframe + corner markers
	{
		for (int ty = g_selTY; ty < g_selTY + g_selH; ++ty)
		{
			for (int tx = g_selTX; tx < g_selTX + g_selW; ++tx)
			{
				if (!g_tileMap.InBounds(tx, ty)) continue;
				auto& t = g_tileMap.Get(tx, ty);
				float fx = static_cast<float>(tx);
				float fz = static_cast<float>(ty);

				glm::vec3 cb[4] = {
					{ fx - 0.5f, t.floorHeight + t.slopeNW, fz - 0.5f },
					{ fx + 0.5f, t.floorHeight + t.slopeNE, fz - 0.5f },
					{ fx + 0.5f, t.floorHeight + t.slopeSE, fz + 0.5f },
					{ fx - 0.5f, t.floorHeight + t.slopeSW, fz + 0.5f },
				};
				glm::vec3 ct[4] = {
					{ fx - 0.5f, t.ceilHeight + t.ceilSlopeNW, fz - 0.5f },
					{ fx + 0.5f, t.ceilHeight + t.ceilSlopeNE, fz - 0.5f },
					{ fx + 0.5f, t.ceilHeight + t.ceilSlopeSE, fz + 0.5f },
					{ fx - 0.5f, t.ceilHeight + t.ceilSlopeSW, fz + 0.5f },
				};

				// Tile wireframe
				for (int i = 0; i < 4; ++i)
					addLine(cb[i], cb[(i + 1) % 4], wireColor);
				for (int i = 0; i < 4; ++i)
					addLine(ct[i], ct[(i + 1) % 4], wireColor);
				for (int i = 0; i < 4; ++i)
					addLine(cb[i], ct[i], wireColor);

				// VERTEX markers per solid tile
				if (t.spaceType == tile::TileSpaceType::SOLID)
				{
					for (int i = 0; i < 8; ++i)
					{
						bool isFloor = (i < 4);
						const glm::vec3& pos = isFloor ? cb[i] : ct[i - 4];
						addRect(pos, isFloor ? COLOR_CORNER : COLOR_CEIL_CORNER);
					}
				}
			}
		}
	}

	if (lines.empty() && tris.empty()) return;

	static gpu::vao::VertexArrayPtr s_vao;
	static gpu::buffer::BufferPtr s_vbo;
	static size_t s_capacity = 0;

	if (!s_vao)
		s_vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);

	size_t lineVerts = lines.size();
	size_t triVerts  = tris.size();
	size_t totalVerts = lineVerts + triVerts;
	size_t needed = totalVerts * sizeof(gr::MeshVertex);

	if (!s_vbo || s_capacity < needed)
	{
		s_vbo = gpu::buffer::CreateBuffer(needed,
			gpu::buffer::BufferStorageFlag::DynamicStorage, "debug_overlay");
		s_capacity = needed;
	}

	if (lineVerts > 0)
		gpu::buffer::UpdateData(s_vbo, lines.data(), lineVerts * sizeof(gr::MeshVertex), 0);
	if (triVerts > 0)
		gpu::buffer::UpdateData(s_vbo, tris.data(), triVerts * sizeof(gr::MeshVertex),
			lineVerts * sizeof(gr::MeshVertex));

	glLineWidth(2.0f);

	gpu::vao::BindVertexArray(s_vao);
	gpu::cmd::BindVertexBuffer(s_vao, 0, s_vbo, 0, sizeof(gr::MeshVertex));
	gpu::program::BindShaderProgram(g_debugProgram);

	auto vp = g_scene->activeCamera->GetViewProjectionMatrix();
	int loc = gpu::program::GetUniformLocation(g_debugProgram, "u_viewProj");
	gpu::program::SetUniform(g_debugProgram, loc, vp);

	if (lineVerts > 0)
	{
		gpu::cmd::SetTopology(gpu::PrimitiveTopology::LineList);
		gpu::cmd::Draw(static_cast<uint32_t>(lineVerts), 1, 0, 0);
	}
	if (triVerts > 0)
	{
		gpu::cmd::SetTopology(gpu::PrimitiveTopology::TriangleList);
		gpu::cmd::Draw(static_cast<uint32_t>(triVerts), 1, static_cast<uint32_t>(lineVerts), 0);
	}
	gpu::cmd::SetTopology(gpu::PrimitiveTopology::TriangleList);
}

// ---- GameRender ----
void GameRender()
{
	math::Frustum frustum;
	if (g_scene->activeCamera)
		frustum = g_scene->activeCamera->ExtractFrustum();

	if (g_scene->enableShadows)
	{
		for (auto* light : g_scene->lights)
		{
			if (!light->castShadow) continue;
			auto q = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Shadow);
			g_scene->RenderShadowPass(q, *light, g_depthShader);
		}
	}

	gpu::fbo::SwapchainRenderInfo sr{};
	sr.colorLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	sr.clearColorValue[0] = 0.08f;
	sr.clearColorValue[1] = 0.10f;
	sr.clearColorValue[2] = 0.18f;
	sr.clearColorValue[3] = 1.0f;
	sr.depthLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	sr.viewport.drawRect.offset  = { 0, 0 };
	sr.viewport.drawRect.extent  = { window::GetWidth(), window::GetHeight() };
	gpu::cmd::BeginDraw(sr, "MainFrame");

	g_scene->enableFrustumCulling = true;
	auto queue = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Opaque);
	g_scene->RenderOpaquePass(queue, g_program);
	g_scene->RenderTransparentPass(queue, g_program);
	DrawDebugOverlay();
	gpu::cmd::EndDraw();
}
