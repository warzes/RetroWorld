#include "stdafx.h"
#include "Editor.h"
#include <cmath>

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

		// Face hover highlighting
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

	if (g_scene->activeCamera)
		g_scene->activeCamera->aspectRatio = window::GetAspectRatio();

	if (g_dirtyMesh)
		RebuildTileMesh();

	g_scene->Update();
}

// ---- GameFixedUpdate ----
void GameFixedUpdate() {}

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
	gpu::cmd::EndDraw();
}
