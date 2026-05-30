#include "stdafx.h"
#include "Editor.h"
#include <cmath>

namespace
{
	// Fill positions for all 10 Plane-mode control points.
	// Returns the number of positions written (10 in PLANE mode, 4 in VERTEX mode).
	int GetCPPositions(glm::vec3* dst, int maxDst, const tile::Tile& t, float fx, float fz, HeightEditMode mode) noexcept
	{
		float fh = t.floorHeight;
		float ch = t.ceilHeight;
		float avgFloorSlope = (t.slopeNW + t.slopeNE + t.slopeSE + t.slopeSW) * 0.25f;
		float avgCeilSlope = (t.ceilSlopeNW + t.ceilSlopeNE + t.ceilSlopeSE + t.ceilSlopeSW) * 0.25f;

		if (mode == HeightEditMode::PLANE)
		{
			if (maxDst < 10) return 0;
			dst[0] = { fx, fh + avgFloorSlope, fz };                                    // FloorCenter
			dst[1] = { fx, ch + avgCeilSlope, fz };                                     // CeilCenter
			dst[2] = { fx,        fh + (t.slopeNW + t.slopeNE) * 0.5f, fz - 0.5f };      // FloorNorth
			dst[3] = { fx,        fh + (t.slopeSW + t.slopeSE) * 0.5f, fz + 0.5f };      // FloorSouth
			dst[4] = { fx - 0.5f, fh + (t.slopeNW + t.slopeSW) * 0.5f, fz };            // FloorWest
			dst[5] = { fx + 0.5f, fh + (t.slopeNE + t.slopeSE) * 0.5f, fz };            // FloorEast
			dst[6] = { fx,        ch + (t.ceilSlopeNW + t.ceilSlopeNE) * 0.5f, fz - 0.5f }; // CeilNorth
			dst[7] = { fx,        ch + (t.ceilSlopeSW + t.ceilSlopeSE) * 0.5f, fz + 0.5f }; // CeilSouth
			dst[8] = { fx - 0.5f, ch + (t.ceilSlopeNW + t.ceilSlopeSW) * 0.5f, fz };       // CeilWest
			dst[9] = { fx + 0.5f, ch + (t.ceilSlopeNE + t.ceilSlopeSE) * 0.5f, fz };       // CeilEast
			return 10;
		}
		else // VERTEX
		{
			if (maxDst < 4) return 0;
			dst[0] = { fx - 0.5f, fh + t.slopeNW, fz - 0.5f };
			dst[1] = { fx + 0.5f, fh + t.slopeNE, fz - 0.5f };
			dst[2] = { fx + 0.5f, fh + t.slopeSE, fz + 0.5f };
			dst[3] = { fx - 0.5f, fh + t.slopeSW, fz + 0.5f };
			return 4;
		}
	}

	void ApplyHeightStep(tile::Tile& t, int cpType, float delta) noexcept
	{
		switch (static_cast<CPType>(cpType))
		{
		case CPType::FloorCenter: t.floorHeight += delta; break;
		case CPType::CeilCenter:  t.ceilHeight  += delta; break;
		case CPType::FloorNorth:   t.slopeNW += delta; t.slopeNE += delta; break;
		case CPType::CeilNorth:    t.ceilSlopeNW += delta; t.ceilSlopeNE += delta; break;
		case CPType::FloorSouth:   t.slopeSE += delta; t.slopeSW += delta; break;
		case CPType::CeilSouth:    t.ceilSlopeSE += delta; t.ceilSlopeSW += delta; break;
		case CPType::FloorWest:    t.slopeNW += delta; t.slopeSW += delta; break;
		case CPType::CeilWest:     t.ceilSlopeNW += delta; t.ceilSlopeSW += delta; break;
		case CPType::FloorEast:    t.slopeNE += delta; t.slopeSE += delta; break;
		case CPType::CeilEast:     t.ceilSlopeNE += delta; t.ceilSlopeSE += delta; break;
		default: break;
		}
	}

	// Marker colors
	constexpr glm::vec4 COLOR_CENTER(1.0f, 1.0f, 0.0f, 1.0f);  // yellow for center markers
	constexpr glm::vec4 COLOR_EDGE(0.3f, 0.6f, 1.0f, 1.0f);    // blue for edge markers
	constexpr glm::vec4 COLOR_HOVER(1.0f, 1.0f, 1.0f, 1.0f);   // white
	constexpr glm::vec4 COLOR_CORNER(0.3f, 1.0f, 0.3f, 1.0f);  // green
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

	// Atlas texture
	g_atlasTex = tile::CreateTileAtlas(64, 8);
	gpu::texture::SamplerState ss{};
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
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
		g_selFace = tile::FaceDir::COUNT;
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
			float fx = static_cast<float>(g_selTX);
			float fz = static_cast<float>(g_selTY);

			glm::vec3 cps[10];
			int n = GetCPPositions(cps, 10, t, fx, fz, g_heightEditMode);

			float bestDist = 0.28f; // hit radius
			for (int i = 0; i < n; ++i)
			{
				glm::vec3 d = cps[i] - camPos;
				float td = glm::dot(d, rayDir);
				if (td <= 0) continue;
				float dist = glm::distance(cps[i], camPos + rayDir * td);
				if (dist < bestDist)
				{
					bestDist = dist;
					g_hoverCPIdx = i;
				}
			}
		}

		// --- Left click ---
		if (lmbPressed && g_scene->activeCamera)
		{
			bool cpPicked = false;

			if (g_hoverCPIdx >= 0)
			{
				// Start dragging the hovered CP
				auto& t = g_tileMap.Get(g_selTX, g_selTY);
				g_draggingCP = true;
				g_dragCPType = g_hoverCPIdx;
				g_dragStartMouseY = mousePos.y;
				g_lastAppliedDy = 0;
				g_dragSlopes[0] = t.slopeNW;
				g_dragSlopes[1] = t.slopeNE;
				g_dragSlopes[2] = t.slopeSE;
				g_dragSlopes[3] = t.slopeSW;
				cpPicked = true;
			}

			if (!cpPicked)
				PickTile(camPos, rayDir);
		}

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
				auto& t = g_tileMap.Get(g_selTX, g_selTY);

				if (g_dragCPType < static_cast<int>(CPType::Corner))
				{
					// Plane mode CP types
					ApplyHeightStep(t, g_dragCPType, delta);
				}
				else
				{
					// VERTEX mode corner
					float* dst[4] = { &t.slopeNW, &t.slopeNE, &t.slopeSE, &t.slopeSW };
					*dst[g_dragCorner] = g_dragSlopes[g_dragCorner] + snappedDy;
				}
				g_lastAppliedDy = snappedDy;
				g_dirtyMesh = true;
			}
		}

		// Cancel CP drag on LMB release
		if (!lmb && g_draggingCP)
			g_draggingCP = false;

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
					switch (g_selCorner)
					{
						case 0: t.slopeNW += delta; break;
						case 1: t.slopeNE += delta; break;
						case 2: t.slopeSE += delta; break;
						case 3: t.slopeSW += delta; break;
					}
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

	auto& t = g_tileMap.Get(g_selTX, g_selTY);
	float fx = static_cast<float>(g_selTX);
	float fz = static_cast<float>(g_selTY);
	float fh = t.floorHeight;
	float ch = t.ceilHeight;

	// 8 corners of the selected tile
	glm::vec3 cb[4] = {
		{ fx - 0.5f, fh + t.slopeNW, fz - 0.5f },
		{ fx + 0.5f, fh + t.slopeNE, fz - 0.5f },
		{ fx + 0.5f, fh + t.slopeSE, fz + 0.5f },
		{ fx - 0.5f, fh + t.slopeSW, fz + 0.5f },
	};
	glm::vec3 ct[4] = {
		{ fx - 0.5f, ch + t.ceilSlopeNW, fz - 0.5f },
		{ fx + 0.5f, ch + t.ceilSlopeNE, fz - 0.5f },
		{ fx + 0.5f, ch + t.ceilSlopeSE, fz + 0.5f },
		{ fx - 0.5f, ch + t.ceilSlopeSW, fz + 0.5f },
	};

	// Camera basis vectors for billboarding (extracted from view matrix)
	glm::mat4 camView = g_scene->activeCamera->GetViewMatrix();
	glm::vec3 camRight = glm::normalize(glm::vec3(camView[0][0], camView[1][0], camView[2][0]));
	glm::vec3 camUp    = glm::normalize(glm::vec3(camView[0][1], camView[1][1], camView[2][1]));
	const float markerHalf = 0.13f; // half-size of marker rectangle

	std::vector<gr::MeshVertex> lines;
	std::vector<gr::MeshVertex> tris;
	glm::vec4 wireColor(0.8f, 0.8f, 0.8f, 0.6f);

	auto addLine = [&](glm::vec3 a, glm::vec3 b, glm::vec4 c) {
		lines.push_back({ a, {}, {}, c });
		lines.push_back({ b, {}, {}, c });
	};

	// Wireframe: bottom face
	for (int i = 0; i < 4; ++i)
		addLine(cb[i], cb[(i + 1) % 4], wireColor);
	// Wireframe: top face
	for (int i = 0; i < 4; ++i)
		addLine(ct[i], ct[(i + 1) % 4], wireColor);
	// Wireframe: vertical edges
	for (int i = 0; i < 4; ++i)
		addLine(cb[i], ct[i], wireColor);

	// Helper: add a camera-facing rectangle as 2 triangles
	auto addRect = [&](const glm::vec3& center, const glm::vec4& color) {
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

	// Control point markers
	if (g_heightEditMode == HeightEditMode::PLANE)
	{
		glm::vec3 cps[10];
		int n = GetCPPositions(cps, 10, t, fx, fz, HeightEditMode::PLANE);

		// Center markers (0,1): yellow; Edge markers (2-9): blue; Hovered: white
		for (int i = 0; i < n; ++i)
		{
			bool isCenter = (i <= static_cast<int>(CPType::CeilCenter));
			bool hovered = (g_hoverCPIdx == i);
			glm::vec4 col = hovered ? COLOR_HOVER : (isCenter ? COLOR_CENTER : COLOR_EDGE);
			addRect(cps[i], col);
		}
	}
	else // VERTEX
	{
		// Corner markers: green
		for (int i = 0; i < 4; ++i)
		{
			bool hovered = (g_hoverCPIdx == i);
			glm::vec4 col = hovered ? COLOR_HOVER : COLOR_CORNER;
			addRect(cb[i], col);
		}
	}

	if (lines.empty() && tris.empty()) return;

	static gpu::vao::VertexArrayPtr s_vao;
	static gpu::buffer::BufferPtr s_vbo;
	static size_t s_capacity = 0;

	if (!s_vao)
		s_vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);

	// Gather all verts into one buffer (lines first, then tris)
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

	// Upload lines
	if (lineVerts > 0)
		gpu::buffer::UpdateData(s_vbo, lines.data(), lineVerts * sizeof(gr::MeshVertex), 0);
	// Upload tris (appended after lines)
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

	// Draw lines
	if (lineVerts > 0)
	{
		gpu::cmd::SetTopology(gpu::PrimitiveTopology::LineList);
		gpu::cmd::Draw(static_cast<uint32_t>(lineVerts), 1, 0, 0);
	}
	// Draw triangles
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
