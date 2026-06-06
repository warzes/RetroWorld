#include "stdafx.h"
#include "MapEditor.h"
//=============================================================================
using namespace ::map;

namespace
{

	//=== Shaders (reusing from existing editor) ==============================
	const char* blinnPhongVert = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

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

void main()
{
	mat4 model = u_isInstanced ? u_instanceData.models[gl_InstanceID] : u_model;
	mat3 normalMatrix = u_isInstanced ? transpose(inverse(mat3(model))) : u_normalMatrix;

	vec4 worldPos = model * vec4(a_position, 1.0);
	v_worldPos    = worldPos.xyz;
	v_worldNormal = normalize(normalMatrix * a_normal);
	v_texcoord    = a_texcoord;

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
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(u_shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth < pcfDepth ? 1.0 : 0.0;
		}
	}
	return shadow / 9.0;
}

float PointShadowCalculation(vec3 fragToLight, float bias, float farPlane)
{
	float closestDepth = texture(u_pointShadowMap, normalize(fragToLight)).r;
	float currentDepth = length(fragToLight) / farPlane;
	return (currentDepth - bias) < closestDepth ? 1.0 : 0.0;
}

vec3 CalcDirectionalLight(LightData light, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
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
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient * albedo + (diffuse + spec) * shadow) * light.intensity;
}

vec3 CalcPointLight(LightData light, vec3 fragPos, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3  L       = light.positionOrDirection.xyz - fragPos;
	float dist    = length(L);
	if (dist > light.radius) return vec3(0.0);
	L /= dist;

	float atten = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);
	float fade  = 1.0 - (dist * dist) / (light.radius * light.radius);
	atten *= fade * fade;

	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	vec3 diffuse = light.color * albedo * NdotL;
	vec3 spec    = light.color * specular * pow(NdotH, shininess);

	return (ambient * albedo + diffuse + spec) * light.intensity * atten;
}

void main()
{
	vec3 albedo   = u_albedoColor;
	vec3 ambient  = u_ambientColor;
	vec3 normal   = normalize(v_worldNormal);

	if (u_hasAlbedoMap) albedo *= texture(u_albedoMap, v_texcoord).rgb;

	vec3 V = normalize(u_cameraPos - v_worldPos);

	vec3 result = vec3(0.0);

	for (int i = 0; i < u_lightCount && i < MAX_LIGHTS; ++i)
	{
		LightData light = u_lights[i];
		if (light.type == 0)
			result += CalcDirectionalLight(light, normal, V, albedo, vec3(0.3), ambient, u_shininess);
		else if (light.type == 1)
			result += CalcPointLight(light, v_worldPos, normal, V, albedo, vec3(0.3), ambient, u_shininess);
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

	const char* shadowDepthFrag = R"(
#version 460 core
void main() {}
)";

	//--- Engine resources ---
	gpu::program::ShaderProgramPtr g_program;
	gpu::program::ShaderProgramPtr g_depthShader;

	std::unique_ptr<scene::SceneManager> g_scene;
	std::unique_ptr<map::MapEditor>      g_editor;

	gr::Camera g_camera(
		glm::vec3(35.0f, 30.0f, -40.0f),
		glm::vec3(30.0f, 0.0f, 30.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	input::MouseLook g_mouseLook;

	// Hover highlight
	int g_hoverGx = -1;
	int g_hoverGz = -1;
	bool g_hoverValid = false;

	// Mesh for grid highlight (wireframe cube on hovered cell)
	std::unique_ptr<gr::Mesh> g_highlightMesh;
	scene::ModelNode* g_highlightNode = nullptr;

	// Reusable vertex/index buffers for highlight
	std::vector<gr::MeshVertex> g_hlVerts;
	std::vector<uint32_t> g_hlIndices;
}

//=============================================================================
static bool RayPlaneIntersection(
	const glm::vec3& rayOrigin, const glm::vec3& rayDir,
	float planeY, float& outT)
{
	if (std::abs(rayDir.y) < 1e-6f) return false;
	outT = (planeY - rayOrigin.y) / rayDir.y;
	return outT >= 0.0f;
}

//=============================================================================
static bool ScreenToGrid(
	int sx, int sy,
	const glm::mat4& view, const glm::mat4& proj,
	int& outGx, int& outGz)
{
	float vpW = static_cast<float>(window::GetWidth());
	float vpH = static_cast<float>(window::GetHeight());

	float ndcX = (2.0f * static_cast<float>(sx) / vpW - 1.0f);
	float ndcY = (1.0f - 2.0f * static_cast<float>(sy) / vpH);

	glm::mat4 invVP = glm::inverse(proj * view);

	// Near and far points in NDC
	glm::vec4 nearNDC(ndcX, ndcY, -1.0f, 1.0f);
	glm::vec4 farNDC(ndcX, ndcY, 1.0f, 1.0f);

	glm::vec4 nearWorld = invVP * nearNDC;
	glm::vec4 farWorld  = invVP * farNDC;

	if (std::abs(nearWorld.w) < 1e-6f) return false;
	if (std::abs(farWorld.w) < 1e-6f) return false;

	nearWorld /= nearWorld.w;
	farWorld  /= farWorld.w;

	glm::vec3 rayOrigin = glm::vec3(nearWorld);
	glm::vec3 rayDir = glm::normalize(glm::vec3(farWorld) - rayOrigin);

	float t;
	if (!RayPlaneIntersection(rayOrigin, rayDir, 0.0f, t))
		return false;

	glm::vec3 hit = rayOrigin + rayDir * t;

	outGx = static_cast<int>(floorf(hit.x));
	outGz = static_cast<int>(floorf(hit.z));

	return true;
}

//=============================================================================
static void BuildHighlightMesh(int gx, int gz)
{
	if (!g_highlightNode || !g_highlightNode->mesh) return;

	g_hlVerts.clear();
	g_hlIndices.clear();

	float fx = static_cast<float>(gx);
	float fz = static_cast<float>(gz);
	float h = 0.05f; // slightly above ground

	// Flat quad on top of cell (2 triangles, CCW from above)
	glm::vec4 hlColor(0.0f, 1.0f, 1.0f, 1.0f);

	g_hlVerts.push_back({.position = {fx,     h, fz     }, .normal = {0,1,0}, .uv = {0,0}, .color = hlColor});
	g_hlVerts.push_back({.position = {fx,     h, fz+1.0f}, .normal = {0,1,0}, .uv = {0,1}, .color = hlColor});
	g_hlVerts.push_back({.position = {fx+1.0f, h, fz+1.0f}, .normal = {0,1,0}, .uv = {1,1}, .color = hlColor});
	g_hlVerts.push_back({.position = {fx+1.0f, h, fz     }, .normal = {0,1,0}, .uv = {1,0}, .color = hlColor});

	g_hlIndices.push_back(0);
	g_hlIndices.push_back(1);
	g_hlIndices.push_back(2);
	g_hlIndices.push_back(0);
	g_hlIndices.push_back(2);
	g_hlIndices.push_back(3);

	auto& mesh = *g_highlightNode->mesh;
	mesh.vertexCount = static_cast<uint32_t>(g_hlVerts.size());
	mesh.indexCount = static_cast<uint32_t>(g_hlIndices.size());
	mesh.isIndexed = true;

	gpu::buffer::UpdateData(mesh.vbo, g_hlVerts.data(), g_hlVerts.size() * sizeof(gr::MeshVertex));
	gpu::buffer::UpdateData(mesh.ibo, g_hlIndices.data(), g_hlIndices.size() * sizeof(uint32_t));
}

//=============================================================================
static bool GameInit()
{
	// Compile shaders
	{
		gpu::program::GraphicsProgramCreateInfo ci{
			.name = "BlinnPhong",
			.vertexShaderCode = blinnPhongVert,
			.fragmentShaderCode = blinnPhongFrag };
		g_program = gpu::program::CreateShaderProgram(ci);
		if (!gpu::program::IsValid(g_program))
		{
			core::Error("Editor: failed to compile BlinnPhong shader");
			return false;
		}
	}
	{
		gpu::program::GraphicsProgramCreateInfo ci{
			.name = "ShadowDepth",
			.vertexShaderCode = shadowDepthVert,
			.fragmentShaderCode = shadowDepthFrag,
		};
		g_depthShader = gpu::program::CreateShaderProgram(ci);
		if (!gpu::program::IsValid(g_depthShader))
		{
			core::Error("Editor: failed to compile ShadowDepth shader");
			return false;
		}
	}

	// Scene manager
	g_scene = std::make_unique<scene::SceneManager>();
	auto& root = *g_scene->root;

	// Camera
	auto& cam = root.AddChild<scene::CameraNode>("camera");
	cam.aspectRatio = window::GetAspectRatio();
	cam.externalCamera = &g_camera;
	g_scene->SetActiveCamera(cam);

	// Lighting
	{
		auto& sun = root.AddChild<scene::LightNode>("sun");
		sun.lightType = scene::LightNode::LightType::Directional;
		sun.color = glm::vec3(1.0f, 0.95f, 0.85f);
		sun.intensity = 1.5f;
		sun.castShadow = true;
		sun.shadowSettings.resolution = 2048;
		sun.shadowSettings.orthoSize = 100.0f;
		sun.shadowSettings.cascadeDistance[0] = -1.0f;
		sun.shadowSettings.cascadeDistance[1] = -1.0f;
		sun.shadowSettings.cascadeDistance[2] = -1.0f;
		sun.shadowSettings.cascadeDistance[3] = -1.0f;
		sun.transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));
	}

	// Ambient fill light (dim point)
	{
		auto& fill = root.AddChild<scene::LightNode>("fill");
		fill.lightType = scene::LightNode::LightType::Point;
		fill.color = glm::vec3(0.6f, 0.7f, 1.0f);
		fill.intensity = 0.5f;
		fill.radius = 200.0f;
		fill.attenuation = glm::vec3(1.0f, 0.01f, 0.001f);
		fill.transform.position = glm::vec3(30.0f, 20.0f, 30.0f);
		fill.castShadow = false;
	}

	// Map editor
	g_editor = std::make_unique<map::MapEditor>();
	g_editor->Init();
	g_editor->RebuildGeometry(*g_scene);

	// Highlight node (wireframe)
	{
		g_highlightMesh = std::make_unique<gr::Mesh>();
		g_highlightMesh->vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);

		// Create empty VBO/IBO
		g_hlVerts.reserve(24);
		g_hlIndices.reserve(24);
		g_highlightMesh->vbo = gpu::buffer::CreateBuffer(
			sizeof(gr::MeshVertex) * 24,
			gpu::buffer::BufferStorageFlag::DynamicStorage);
		g_highlightMesh->ibo = gpu::buffer::CreateBuffer(
			sizeof(uint32_t) * 24,
			gpu::buffer::BufferStorageFlag::DynamicStorage);

		g_highlightMesh->vertexCount = 0;
		g_highlightMesh->indexCount = 0;
		g_highlightMesh->isIndexed = true;

		auto& hlNode = root.AddChild<scene::ModelNode>("highlight");
		hlNode.mesh = std::make_shared<gr::Mesh>(*g_highlightMesh);
		hlNode.material = std::make_shared<gr::Material>();
		hlNode.material->albedoColor = glm::vec3(0.0f, 1.0f, 1.0f);
		hlNode.material->specularColor = glm::vec3(0.0f);
		hlNode.material->ambientColor = glm::vec3(1.0f);
		hlNode.material->shininess = 1.0f;
		// No culling so wireframe shows from any angle
		hlNode.material->cullMode = gpu::CullMode::None;
		hlNode.castShadow = false;
		hlNode.receiveShadow = false;

		g_highlightNode = static_cast<scene::ModelNode*>(root.FindChild("highlight"));
	}

	g_scene->enableShadows = true;
	g_scene->enableInstancing = true;

	return true;
}

//=============================================================================
static void GameClose()
{
	g_highlightNode = nullptr;
	g_highlightMesh.reset();
	g_editor.reset();
	g_scene.reset();
	g_mouseLook.Reset();
	g_program.reset();
	g_depthShader.reset();
}

//=============================================================================
static void GameUpdate()
{
	// Camera movement (WASD)
	const float speed = 15.0f * app::GetDeltaTime();
	if (input::IsKeyDown(KeyboardType::KEY_W)) g_camera.Move(gr::Movement::Forward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_S)) g_camera.Move(gr::Movement::Backward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_A)) g_camera.Move(gr::Movement::Left, speed);
	if (input::IsKeyDown(KeyboardType::KEY_D)) g_camera.Move(gr::Movement::Right, speed);
	if (input::IsKeyDown(KeyboardType::KEY_Q)) g_camera.Move(gr::Movement::Down, speed);
	if (input::IsKeyDown(KeyboardType::KEY_E)) g_camera.Move(gr::Movement::Up, speed);

	// Mouse look (right button)
	if (!ImGui::GetIO().WantCaptureMouse && input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
		g_mouseLook.OnRightDown();
	else
		g_mouseLook.OnRightUp();
	g_mouseLook.Update(g_camera);

	// Update camera aspect
	if (g_scene->activeCamera)
		g_scene->activeCamera->aspectRatio = window::GetAspectRatio();

	// Update editor
	g_editor->Update(*g_scene);

	// Mouse pick: raycast to y=0 plane
	g_hoverValid = false;
	if (!ImGui::GetIO().WantCaptureMouse)
	{
		int mx = static_cast<int>(input::GetMousePosition().x);
		int my = static_cast<int>(input::GetMousePosition().y);

		int gx, gz;
		glm::mat4 view = g_camera.GetViewMatrix();
		glm::mat4 proj = g_scene->activeCamera
			? g_scene->activeCamera->GetProjectionMatrix()
			: glm::mat4(1.0f);

		if (ScreenToGrid(mx, my, view, proj, gx, gz))
		{
			if (gx >= 0 && gx < map::MAP_SIZE && gz >= 0 && gz < map::MAP_SIZE)
			{
				g_hoverGx = gx;
				g_hoverGz = gz;
				g_hoverValid = true;

				// Paint on left click
				if (input::IsMouseDown(MouseType::MOUSE_BUTTON_LEFT))
				{
					// PaintCell/EraseCell handles the tool
					if (g_editor)
					{
						if (input::IsKeyDown(KeyboardType::KEY_LEFT_SHIFT))
							g_editor->EraseCell(gx, gz);
						else
							g_editor->PaintCell(gx, gz);
					}
				}
			}
		}
	}

	// Update highlight mesh
	if (g_hoverValid && g_highlightNode && g_highlightNode->mesh)
	{
		BuildHighlightMesh(g_hoverGx, g_hoverGz);
		g_highlightNode->visible = true;
	}
	else if (g_highlightNode)
	{
		g_highlightNode->visible = false;
	}

	// Update scene graph (collect lights, update world matrices)
	g_scene->Update();
}

//=============================================================================
static void GameFixedUpdate()
{}

//=============================================================================
static void GameRender()
{
	math::Frustum frustum;
	if (g_scene->activeCamera)
		frustum = g_scene->activeCamera->ExtractFrustum();

	// Shadow pass
	if (g_scene->enableShadows)
	{
		for (auto* light : g_scene->lights)
		{
			if (!light->castShadow) continue;
			auto shadowQueue = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Shadow);
			g_scene->RenderShadowPass(shadowQueue, *light, g_depthShader);
		}
	}

	// Main pass
	gpu::fbo::SwapchainRenderInfo swapchainRI = {};
	swapchainRI.colorLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.clearColorValue[0] = 0.30f;
	swapchainRI.clearColorValue[1] = 0.55f;
	swapchainRI.clearColorValue[2] = 0.85f;
	swapchainRI.clearColorValue[3] = 1.0f;
	swapchainRI.depthLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.viewport.drawRect.offset = { 0, 0 };
	swapchainRI.viewport.drawRect.extent = { window::GetWidth(), window::GetHeight() };
	gpu::cmd::BeginDraw(swapchainRI, "MainFrame");

	g_scene->enableFrustumCulling = false;
	auto queue = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Opaque);
	g_scene->RenderOpaquePass(queue, g_program);
	g_scene->RenderTransparentPass(queue, g_program);

	gpu::cmd::EndDraw();
}

//=============================================================================
static void GameRenderUI()
{
	// Stats
	ImGui::Begin("Editor Info");
	ImGui::Text("Camera: (%.1f, %.1f, %.1f)",
		g_camera.GetPosition().x, g_camera.GetPosition().y, g_camera.GetPosition().z);
	ImGui::Text("Draw calls: %u", g_scene->lastFrameStats.drawCalls);
	ImGui::Text("Instanced:  %u", g_scene->lastFrameStats.instancedBatches);
	if (g_hoverValid)
		ImGui::Text("Hover: (%d, %d)", g_hoverGx, g_hoverGz);
	else
		ImGui::Text("Hover: (none)");
	ImGui::Text("Shift+Click = erase");
	ImGui::End();

	// Map editor UI
	if (g_editor)
		g_editor->RenderUI();
}

//=============================================================================
void EditorApp()
{
	app::AppCreateInfo createInfo{};
	createInfo.init_cb = GameInit;
	createInfo.close_cb = GameClose;
	createInfo.update_cb = GameUpdate;
	createInfo.fixedUpdate_cb = GameFixedUpdate;
	createInfo.render_cb = GameRender;
	createInfo.renderUi_cb = GameRenderUI;

	app::Run(createInfo);
}
//=============================================================================
