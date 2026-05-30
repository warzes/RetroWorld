#include "stdafx.h"
#include "tile.h"
//=============================================================================
namespace
{
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

	gpu::program::ShaderProgramPtr g_program;
	gpu::program::ShaderProgramPtr g_depthShader;
	gpu::program::ShaderProgramPtr g_pointDepthShader;

	std::unique_ptr<scene::SceneManager> g_scene;
	gr::Camera g_camera(glm::vec3(0, 10, 0), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	input::MouseLook g_mouseLook;

	// --- Tile system ---
	tile::TileMap g_tileMap;
	tile::TileMeshGen g_tileMeshCPU;
	gr::Mesh g_tileMesh;
	gr::Material g_tileMaterial;
	gpu::texture::TexturePtr g_atlasTex;
	gpu::texture::SamplerPtr g_atlasSampler;

	scene::ModelNode* g_tileModelNode = nullptr;

	// --- Editor state ---
	enum class EditMode { TILE, FACE, VERTEX };
	EditMode g_editMode = EditMode::TILE;
	int  g_selTX = -1, g_selTY = -1;
	tile::FaceDir g_selFace = tile::FaceDir::COUNT;
	int  g_selCorner = -1;
	bool g_dirtyMesh = true;

	int g_brushWallTex   = 0;
	int g_brushFloorTex  = 1;
	int g_brushCeilTex   = 2;
	bool g_brushSolid    = true;

	int g_mapWidth  = 16;
	int g_mapHeight = 16;
	uint32_t g_genSeed = 0;

	// --- Hover state ---
	int g_hoverTX = -1, g_hoverTY = -1;
	tile::FaceDir g_hoverFace = tile::FaceDir::COUNT;
	int g_prevHoverTX = -1, g_prevHoverTY = -1;
	tile::FaceDir g_prevHoverFace = tile::FaceDir::COUNT;
	const glm::vec4 HOVER_PINK{1.5f, 0.3f, 0.8f, 1.0f};
}

//=============================================================================
static void RebuildTileMesh()
{
	g_tileMeshCPU.BuildFromMap(g_tileMap, 4, g_selTX, g_selTY, 6);

	// Apply hover highlighting directly into CPU mesh colors
	if (g_hoverTX >= 0)
	{
		g_tileMeshCPU.SetFaceColor(g_hoverTX, g_hoverTY,
			static_cast<int>(g_hoverFace), HOVER_PINK);
	}

	if (g_tileModelNode)
	{
		g_tileMesh.Close();
		g_tileMesh = g_tileMeshCPU.CreateMesh();
		g_tileModelNode->mesh = std::make_shared<gr::Mesh>(g_tileMesh);
	}
	g_dirtyMesh = false;
}

//=============================================================================
static void PickTile(const glm::vec3& rayOrigin, const glm::vec3& rayDir)
{
	tile::HitInfo hit;
	if (g_tileMeshCPU.RayIntersect(rayOrigin, rayDir, hit))
	{
		g_selTX = hit.tileX;
		g_selTY = hit.tileY;
		g_selFace = hit.face;
		g_selCorner = -1;
		g_dirtyMesh = true;

		if (g_editMode == EditMode::VERTEX)
		{
			auto ci = g_tileMap.FindNearestCorner(hit.point, 0.4f);
			if (ci.dist < 0.4f)
			{
				g_selTX = ci.tx;
				g_selTY = ci.ty;
				g_selCorner = ci.corner;
			}
		}
	}
	else
	{
		g_selTX = g_selTY = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_selCorner = -1;
		g_dirtyMesh = true;
	}
}

//=============================================================================
static bool GameInit()
{
	// --- Shaders ---
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

	// --- Tile map — procedural dungeon ---
	g_tileMap.Resize(g_mapWidth, g_mapHeight);
	g_tileMap.GenerateRandom(++g_genSeed);

	// --- Atlas texture ---
	g_atlasTex = tile::CreateTileAtlas(64, 4);
	gpu::texture::SamplerState ss{};
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
	g_atlasSampler = gpu::texture::CreateSampler(ss);

	// --- Material ---
	g_tileMaterial.albedoMap = g_atlasTex;
	g_tileMaterial.sampler   = g_atlasSampler;
	g_tileMaterial.albedoColor  = { 1, 1, 1 };
	g_tileMaterial.specularColor = { 0.05f, 0.05f, 0.05f };
	g_tileMaterial.ambientColor  = { 0.08f, 0.08f, 0.08f };
	g_tileMaterial.shininess = 8.0f;
	//g_tileMaterial.cullMode = gpu::CullMode::None;

	// --- SceneManager ---
	g_scene = std::make_unique<scene::SceneManager>();
	auto& root = *g_scene->root;

	// Camera
	auto& camNode = root.AddChild<scene::CameraNode>("camera");
	camNode.externalCamera = &g_camera;
	camNode.aspectRatio = window::GetAspectRatio();
	camNode.farPlane = 200.0f;
	g_scene->SetActiveCamera(camNode);

	// Directional light
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

	// Build initial mesh
	RebuildTileMesh();

	// Camera look at center of map
	g_camera = gr::Camera(
		glm::vec3(0.0f, 5.0f, 5.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0, 1, 0));

	g_scene->enableShadows    = true;
	g_scene->enableInstancing = false;
	return true;
}

//=============================================================================
static void GameClose()
{
	g_tileMesh.Close();
	g_pointDepthShader.reset();
	g_depthShader.reset();
	g_program.reset();
	g_atlasTex.reset();
	g_atlasSampler.reset();
	g_scene.reset();
	g_mouseLook.Reset();
}

//=============================================================================
static void GameUpdate()
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

	// Regenerate
	if (input::IsKeyDown(KeyboardType::KEY_R))
	{
		g_tileMap.GenerateRandom(++g_genSeed);
		g_selTX = g_selTY = g_selCorner = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_dirtyMesh = true;
	}

	// Mouse-dependent scene interaction (blocked when ImGui wants the mouse)
	static bool prevLMB = false;
	bool lmb = input::IsMouseDown(MouseType::MOUSE_BUTTON_LEFT);
	bool lmbPressed = lmb && !prevLMB;

	if (!wantCaptureMouse)
	{
		// Left click picking
		if (lmbPressed && g_scene->activeCamera)
		{
			auto mp = input::GetMousePosition();
			auto vp = g_scene->activeCamera->GetViewProjectionMatrix();
			float ww = static_cast<float>(window::GetWidth());
			float wh = static_cast<float>(window::GetHeight());
			glm::vec3 rayDir = tile::ScreenToRay(vp, static_cast<float>(mp.x), static_cast<float>(mp.y), ww, wh);
			glm::vec3 camPos = g_scene->activeCamera->GetPosition();
			PickTile(camPos, rayDir);
		}

		// Face hover highlighting (raycast every frame)
		{
			tile::HitInfo hit;
			if (g_scene->activeCamera)
			{
				auto mp = input::GetMousePosition();
				auto vp = g_scene->activeCamera->GetViewProjectionMatrix();
				float ww = static_cast<float>(window::GetWidth());
				float wh = static_cast<float>(window::GetHeight());
				glm::vec3 rayDir = tile::ScreenToRay(vp, static_cast<float>(mp.x), static_cast<float>(mp.y), ww, wh);
				glm::vec3 camPos = g_scene->activeCamera->GetPosition();

				if (g_tileMeshCPU.RayIntersect(camPos, rayDir, hit))
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

		// Apply brush on click in TILE mode
		if (lmbPressed && g_editMode == EditMode::TILE &&
			g_selTX >= 0 && g_tileMap.InBounds(g_selTX, g_selTY))
		{
			auto& t = g_tileMap.Get(g_selTX, g_selTY);
			if (g_brushSolid)
			{
				t.spaceType   = tile::TileSpaceType::SOLID;
				t.renderSolid = true;
				t.wallTex     = static_cast<uint8_t>(g_brushWallTex);
				t.floorTex    = static_cast<uint8_t>(g_brushFloorTex);
				t.ceilTex     = static_cast<uint8_t>(g_brushCeilTex);
			}
			else
			{
				t.spaceType   = tile::TileSpaceType::EMPTY;
				t.renderSolid = false;
			}
			g_dirtyMesh = true;
		}

		// Scroll to adjust height in VERTEX mode
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

	// Update camera aspect
	if (g_scene->activeCamera)
		g_scene->activeCamera->aspectRatio = window::GetAspectRatio();

	if (g_dirtyMesh)
	{
		RebuildTileMesh();
	}

	g_scene->Update();
}

//=============================================================================
static void GameFixedUpdate() {}

//=============================================================================
static void GameRender()
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
	gpu::cmd::EndDraw();
}

//=============================================================================
static void GameRenderUI()
{
	// --- Main Menu Bar (Delver Engine style) ---
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New", "Ctrl+N")) {}
			if (ImGui::BeginMenu("Open Recent"))
			{
				ImGui::Text("(empty)");
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem("Save", "Ctrl+S")) {}
			if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Exit")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z")) {}
			if (ImGui::MenuItem("Redo", "Ctrl+Y")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Copy", "Ctrl+C")) {}
			if (ImGui::MenuItem("Paste", "Ctrl+V")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Tile"))
		{
			if (ImGui::MenuItem("Carve", "Enter")) {}
			if (ImGui::MenuItem("Paint", "Shift+Enter")) {}
			if (ImGui::MenuItem("Delete", "Del")) {}
			if (ImGui::MenuItem("Deselect", "Escape")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Height Edit Mode"))
			{
				if (ImGui::MenuItem("Plane")) {}
				if (ImGui::MenuItem("Vertex")) {}
				if (ImGui::MenuItem("Toggle", "V")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Raise/Lower"))
			{
				if (ImGui::MenuItem("Raise Floor", "3")) {}
				if (ImGui::MenuItem("Lower Floor", "Shift+3")) {}
				if (ImGui::MenuItem("Raise Ceiling", "4")) {}
				if (ImGui::MenuItem("Lower Ceiling", "Shift+4")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Move"))
			{
				if (ImGui::MenuItem("Move North", "Shift+Up")) {}
				if (ImGui::MenuItem("Move South", "Shift+Down")) {}
				if (ImGui::MenuItem("Move East", "Shift+Left")) {}
				if (ImGui::MenuItem("Move West", "Shift+Right")) {}
				if (ImGui::MenuItem("Move Up", "Shift+E")) {}
				if (ImGui::MenuItem("Move Down", "Shift+Q")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Rotate Wall Angle", "U")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Flatten"))
			{
				if (ImGui::MenuItem("Floor", "F")) {}
				if (ImGui::MenuItem("Ceiling", "Shift+F")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Pick Textures", "G")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Rotate Texture"))
			{
				if (ImGui::MenuItem("Floor", "T")) {}
				if (ImGui::MenuItem("Ceiling", "Shift+T")) {}
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Surface"))
			{
				if (ImGui::MenuItem("Paint Surface Texture", "1")) {}
				if (ImGui::MenuItem("Grab Surface Texture", "2")) {}
				if (ImGui::MenuItem("Pick Surface Texture", "Shift+2")) {}
				if (ImGui::MenuItem("Flood Fill Surface Texture", "Shift+1")) {}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Entity"))
		{
			if (ImGui::MenuItem("Delete", "Del")) {}
			if (ImGui::MenuItem("Deselect", "Escape")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Move"))
			{
				if (ImGui::MenuItem("Constrain to X-axis", "X")) {}
				if (ImGui::MenuItem("Constrain to Y-axis", "Y")) {}
				if (ImGui::MenuItem("Constrain to Z-axis", "Z")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Rotate", "R")) {}

			if (ImGui::BeginMenu("Turn"))
			{
				if (ImGui::MenuItem("Clockwise", "Ctrl+Left")) {}
				if (ImGui::MenuItem("Counter-clockwise", "Ctrl+Right")) {}
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Toggle Simulation", "B")) {}
			if (ImGui::MenuItem("Toggle Gizmos")) {}
			if (ImGui::MenuItem("Toggle Lights", "L")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("View Selected", "Space")) {}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Level"))
		{
			if (ImGui::MenuItem("Play From Camera", "P")) {}
			if (ImGui::MenuItem("Play From Start", "Shift+P")) {}
			ImGui::Separator();

			if (ImGui::BeginMenu("Rotate Level"))
			{
				if (ImGui::MenuItem("Clockwise")) {}
				if (ImGui::MenuItem("Counter-clockwise")) {}
				ImGui::EndMenu();
			}

			if (ImGui::MenuItem("Resize Level")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Set Theme")) {}
			ImGui::Separator();
			if (ImGui::MenuItem("Generate Room")) {}
			if (ImGui::MenuItem("Generate Level")) {}
			ImGui::EndMenu();
		}

		// Play button (right-aligned)
		ImGui::SameLine(ImGui::GetWindowWidth() - 40);
		if (ImGui::Button(">")) {}

		ImGui::EndMainMenuBar();
	}

	// --- Editor panel (tool window) ---
	ImGui::Begin("Tile Editor", nullptr, ImGuiWindowFlags_NoCollapse);

	ImGui::Text("WASD=move  RMB=look  1/2/3=Mode  R=Regen");
	ImGui::Separator();

	// Mode
	int mode = static_cast<int>(g_editMode);
	ImGui::RadioButton("Tile",   &mode, 0); ImGui::SameLine();
	ImGui::RadioButton("Face",   &mode, 1); ImGui::SameLine();
	ImGui::RadioButton("Vertex", &mode, 2);
	g_editMode = static_cast<EditMode>(mode);

	ImGui::Separator();

	if (g_editMode == EditMode::TILE)
	{
		ImGui::Text("Brush:");
		ImGui::Checkbox("Solid", &g_brushSolid);
		ImGui::SliderInt("Wall Tex",  &g_brushWallTex,  0, 15);
		ImGui::SliderInt("Floor Tex", &g_brushFloorTex, 0, 15);
		ImGui::SliderInt("Ceil Tex",  &g_brushCeilTex,  0, 15);

		ImGui::Separator();
		ImGui::Text("Click tile to select, then change brush & click again to place.");

		if (g_selTX >= 0)
		{
			ImGui::Text("Selected: (%d, %d)", g_selTX, g_selTY);
			auto& t = g_tileMap.Get(g_selTX, g_selTY);

			int st = static_cast<int>(t.spaceType);
			ImGui::RadioButton("Empty", &st, 0); ImGui::SameLine();
			ImGui::RadioButton("Solid", &st, 1);
			t.spaceType   = static_cast<tile::TileSpaceType>(st);
			t.renderSolid = (st == 1);

			int wt = t.wallTex, ft = t.floorTex, ct = t.ceilTex;
			if (ImGui::SliderInt("Wall",  &wt, 0, 15)) { t.wallTex  = static_cast<uint8_t>(wt); g_dirtyMesh = true; }
			if (ImGui::SliderInt("Floor", &ft, 0, 15)) { t.floorTex = static_cast<uint8_t>(ft); g_dirtyMesh = true; }
			if (ImGui::SliderInt("Ceil",  &ct, 0, 15)) { t.ceilTex  = static_cast<uint8_t>(ct); g_dirtyMesh = true; }

			if (ImGui::Button("Remove Tile"))
			{
				t.spaceType   = tile::TileSpaceType::EMPTY;
				t.renderSolid = false;
				g_dirtyMesh   = true;
			}
			ImGui::SameLine();
			if (ImGui::Button("Place Tile"))
			{
				t.spaceType   = tile::TileSpaceType::SOLID;
				t.renderSolid = true;
				t.wallTex     = static_cast<uint8_t>(g_brushWallTex);
				t.floorTex    = static_cast<uint8_t>(g_brushFloorTex);
				t.ceilTex     = static_cast<uint8_t>(g_brushCeilTex);
				g_dirtyMesh   = true;
			}
		}
	}

	if (g_editMode == EditMode::FACE)
	{
		if (g_selTX >= 0)
		{
			auto& t = g_tileMap.Get(g_selTX, g_selTY);
			ImGui::Text("Tile: (%d, %d)", g_selTX, g_selTY);
			ImGui::Text("Face: %s", g_selFace < tile::FaceDir::COUNT ?
				tile::FaceNames[static_cast<int>(g_selFace)] : "none");

			if (g_selFace == tile::FaceDir::FLOOR)
			{
				int ft = t.floorTex;
				if (ImGui::SliderInt("Floor Texture", &ft, 0, 15))
				{ t.floorTex = static_cast<uint8_t>(ft); g_dirtyMesh = true; }
			}
			else if (g_selFace == tile::FaceDir::CEILING)
			{
				int ct = t.ceilTex;
				if (ImGui::SliderInt("Ceil Texture", &ct, 0, 15))
				{ t.ceilTex = static_cast<uint8_t>(ct); g_dirtyMesh = true; }
			}
			else
			{
				int wt = t.wallTex;
				if (ImGui::SliderInt("Wall Texture", &wt, 0, 15))
				{ t.wallTex = static_cast<uint8_t>(wt); g_dirtyMesh = true; }
			}
		}
		else ImGui::Text("Click on a face to select.");
	}

	if (g_editMode == EditMode::VERTEX)
	{
		const char* cornerNames[] = { "NW", "NE", "SE", "SW" };
		if (g_selTX >= 0 && g_selCorner >= 0)
		{
			ImGui::Text("Tile: (%d, %d)", g_selTX, g_selTY);
			ImGui::Text("Corner: %s", cornerNames[g_selCorner]);

			auto& t = g_tileMap.Get(g_selTX, g_selTY);
			float* slope = nullptr;
			switch (g_selCorner)
			{
				case 0: slope = &t.slopeNW; break;
				case 1: slope = &t.slopeNE; break;
				case 2: slope = &t.slopeSE; break;
				case 3: slope = &t.slopeSW; break;
			}
			if (slope)
			{
				float val = *slope;
				if (ImGui::SliderFloat("Height Offset", &val, -1.0f, 1.0f))
				{ *slope = val; g_dirtyMesh = true; }
			}
			ImGui::Text("Scroll to adjust height.");
		}
		else ImGui::Text("Click near a corner to select.");
	}

	ImGui::Separator();
	ImGui::Text("Map: %d x %d", g_mapWidth, g_mapHeight);
	if (ImGui::Button("Regenerate"))
	{
		g_tileMap.GenerateRandom(++g_genSeed);
		g_selTX = g_selTY = g_selCorner = -1;
		g_selFace = tile::FaceDir::COUNT;
		g_dirtyMesh = true;
	}

	// Stats
	ImGui::Separator();
	ImGui::Text("Verts: %zu", g_tileMeshCPU.positions.size());
	ImGui::Text("Tris:  %zu", g_tileMeshCPU.indices.size() / 3);
	ImGui::Text("Draw calls: %u", g_scene->lastFrameStats.drawCalls);

	ImGui::End();
}

//=============================================================================
void GameAppTile()
{
	app::AppCreateInfo ci{};
	ci.init_cb        = GameInit;
	ci.close_cb       = GameClose;
	ci.update_cb      = GameUpdate;
	ci.fixedUpdate_cb = GameFixedUpdate;
	ci.render_cb      = GameRender;
	ci.renderUi_cb    = GameRenderUI;
	app::Run(ci);
}
//=============================================================================
