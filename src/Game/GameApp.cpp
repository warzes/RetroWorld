#include "stdafx.h"
//=============================================================================
namespace
{
	const char* vertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normalMatrix;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_worldNormal;
layout(location = 2) out vec2 v_texcoord;

void main()
{
	vec4 worldPos = u_model * vec4(a_position, 1.0);
	v_worldPos    = worldPos.xyz;
	v_worldNormal = normalize(u_normalMatrix * a_normal);
	v_texcoord    = a_texcoord;

	gl_Position = u_projection * u_view * worldPos;
}
)";

	const char* fragmentSource = R"(
#version 460 core

#define MAX_LIGHTS 16

struct LightData {
	vec4  positionOrDirection; // w=0 directional, w=1 point, w=2 spot
	vec3  color;
	float intensity;
	vec3  attenuation;         // constant, linear, quadratic
	float radius;
	vec3  spotDirection;
	float innerCutoff;         // cos(innerAngle)
	float outerCutoff;         // cos(outerAngle)
	int   type;                // 0=directional, 1=point, 2=spot
	bool  castShadow;
	float shadowBias;
	mat4  lightSpaceMatrix;
};

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec2 v_texcoord;

uniform LightData  u_lights[MAX_LIGHTS];
uniform int        u_lightCount;
uniform vec3       u_cameraPos;

// Material uniforms
uniform vec3  u_albedoColor;
uniform vec3  u_specularColor;
uniform vec3  u_ambientColor;
uniform float u_shininess;
uniform float u_opacity;

// Texture uniforms (sampler handles, 0 = no texture)
uniform bool       u_hasAlbedoMap;
uniform bool       u_hasNormalMap;
uniform bool       u_hasSpecularMap;
uniform bool       u_hasEmissiveMap;
layout(binding = 0) uniform sampler2D u_albedoMap;
layout(binding = 1) uniform sampler2D u_normalMap;
layout(binding = 2) uniform sampler2D u_specularMap;
layout(binding = 3) uniform sampler2D u_emissiveMap;

// Shadow
uniform bool       u_receiveShadow;

layout(location = 0) out vec4 o_color;

vec3 CalcDirectionalLight(LightData light, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3 L = normalize(-light.spotDirection);
	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient + diffuse + spec) * light.intensity;
}

vec3 CalcPointLight(LightData light, vec3 fragPos, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3  L       = light.positionOrDirection.xyz - fragPos;
	float dist    = length(L);
	if (dist > light.radius) return vec3(0.0);
	L /= dist;

	float atten = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);

	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient + diffuse + spec) * light.intensity * atten;
}

vec3 CalcSpotLight(LightData light, vec3 fragPos, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3  L       = light.positionOrDirection.xyz - fragPos;
	float dist    = length(L);
	if (dist > light.radius) return vec3(0.0);
	L /= dist;

	float theta     = dot(L, normalize(-light.spotDirection));
	float epsilon   = light.innerCutoff - light.outerCutoff;
	float spot      = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

	float atten = spot / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);

	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient + diffuse + spec) * light.intensity * atten;
}

void main()
{
	vec3 albedo   = u_albedoColor;
	vec3 specular = u_specularColor;
	vec3 ambient  = u_ambientColor;
	vec3 normal   = normalize(v_worldNormal);

	if (u_hasAlbedoMap)   albedo   *= texture(u_albedoMap,   v_texcoord).rgb;
	if (u_hasSpecularMap) specular *= texture(u_specularMap, v_texcoord).rgb;
	if (u_hasNormalMap)
	{
		vec3 n = texture(u_normalMap, v_texcoord).xyz * 2.0 - 1.0;
		// Simple tangent-space normal: derive TBN from screen-space derivatives
		vec3 dx = dFdx(v_worldPos);
		vec3 dy = dFdy(v_worldPos);
		vec3 T  = normalize(dx * n.x + dy * n.y);
		vec3 B  = normalize(cross(normal, T));
		vec3 N  = normalize(cross(T, B));
		mat3 TBN = mat3(T, B, N);
		normal = normalize(TBN * n);
	}

	vec3 V = normalize(u_cameraPos - v_worldPos);

	vec3 result = vec3(0.0);

	for (int i = 0; i < u_lightCount && i < MAX_LIGHTS; ++i)
	{
		LightData light = u_lights[i];

		if (light.type == 0)
			result += CalcDirectionalLight(light, normal, V, albedo, specular, ambient, u_shininess);
		else if (light.type == 1)
			result += CalcPointLight(light, v_worldPos, normal, V, albedo, specular, ambient, u_shininess);
		else if (light.type == 2)
			result += CalcSpotLight(light, v_worldPos, normal, V, albedo, specular, ambient, u_shininess);
	}

	// Emissive
	if (u_hasEmissiveMap)
		result += texture(u_emissiveMap, v_texcoord).rgb;

	o_color = vec4(result, u_opacity);
	//o_color = texture(u_albedoMap, v_texcoord);
}
)";

	gpu::program::ShaderProgramPtr program;
	gr::Mesh mesh;
	gr::Material material;

	std::unique_ptr<scene::SceneManager> g_scene;

	gpu::uniform::Uniform<glm::mat4> model;
	gpu::uniform::Uniform<glm::mat4> view;
	gpu::uniform::Uniform<glm::mat4> proj;
	gpu::uniform::Uniform<glm::mat3> normalMatrix;

	gpu::DepthState depthState;

	gr::Camera camera(glm::vec3(0.4f, 1.2f, -2.4f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// Helper: set CameraNode transform from eye/target/up
void SetupCameraLookAt(scene::CameraNode& cam,
	const glm::vec3& eye,
	const glm::vec3& target,
	const glm::vec3& up)
{
	cam.transform.position = eye;
	glm::mat4 view = glm::lookAt(eye, target, up);
	glm::mat4 world = glm::inverse(view);
	cam.transform.rotation = glm::quat_cast(glm::mat3(world));
}

// === WinAPI mouse-look for first-person camera ===
// Handles: focus loss (Alt+Tab), minimize, cursor clipping, synthetic WM_MOUSEMOVE filtering
class MouseLook final
{
public:
	// Called every frame from GameUpdate
	void Update(gr::Camera& cam)
	{
		// Auto-release on focus loss or minimize
		if (m_captured && !CanCapture())
		{
			EndCapture();
			return;
		}

		// Auto-recapture when focus regained (if button still held)
		if (!m_captured && m_wantCapture && CanCapture())
		{
			BeginCapture();
			if (!m_captured) return;
		}

		if (!m_captured) return;

		// Compute raw mouse delta since last warp
		math::point2 current = input::GetMousePosition();
		int dx = current.x - m_centerX;
		int dy = current.y - m_centerY;

		// switch center to window center so player has room to turn both ways
		m_centerX = window::GetWidth() / 2;
		m_centerY = window::GetHeight() / 2;

		// Warp cursor back to new center
		input::SetMousePosition(m_centerX, m_centerY);
		
		// Apply camera rotation (scale by sensitivity)
		if (dx != 0 || dy != 0)		
			cam.Rotate(-static_cast<float>(dy) * 0.1f, static_cast<float>(dx) * 0.1f, 0.0f);
	}

	// Call when right mouse button goes down
	void OnRightDown()
	{
		m_wantCapture = true;
		if (CanCapture())
			BeginCapture();
	}

	// Call when right mouse button goes up
	void OnRightUp()
	{
		m_wantCapture = false;
		EndCapture();
	}

	// Drop everything (e.g. on game close)
	void Reset()
	{
		m_wantCapture = false;
		EndCapture();
	}

private:
	bool        m_captured = false;
	bool        m_wantCapture = false;
	int         m_centerX = 0;
	int         m_centerY = 0;

	bool CanCapture() const
	{
		return window::GetWindowActive() && !window::GetWindowMinimized();
	}

	void BeginCapture()
	{
		if (m_captured) return;

		// use current cursor position as center so dx/dy are zero on this frame
		math::point2 current = input::GetMousePosition();
		m_centerX = current.x;
		m_centerY = current.y;

		// Hide cursor, capture mouse to window, clip to client area
		input::CaptureMause(true);
		// Center cursor so first delta is zero
		input::SetMousePosition(m_centerX, m_centerY);

		m_captured = true;
	}

	void EndCapture()
	{
		if (!m_captured) return;
		m_captured = false;

		input::CaptureMause(false);
	}
};

static MouseLook g_mouseLook;

//=============================================================================
bool GameInit()
{
	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;

	gpu::program::GraphicsProgramCreateInfo createInfo{
		.name               = "Program",
		.vertexShaderCode   = vertexSource,
		.fragmentShaderCode = fragmentSource };
	program = gpu::program::CreateShaderProgram(createInfo);

	gpu::uniform::InitUniform(model, program, "u_model");
	gpu::uniform::InitUniform(view, program, "u_view");
	gpu::uniform::InitUniform(proj, program, "u_projection");
	gpu::uniform::InitUniform(normalMatrix, program, "u_normalMatrix");

	mesh = gr::Mesh::CreateCube();
	material.albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");

	// --- SceneManager ---
	g_scene = std::make_unique<scene::SceneManager>();
	auto& root = *g_scene->root;

	// --- Camera ---
	auto& cam = root.AddChild<scene::CameraNode>("camera");
	SetupCameraLookAt(cam,
		glm::vec3(0.0f, 1.2f, -3.0f),  // eye
		glm::vec3(0.0f, 0.0f, 0.0f),  // target
		glm::vec3(0.0f, 1.0f, 0.0f)); // up
	cam.aspectRatio = window::GetAspectRatio();
	cam.externalCamera = &camera;

	g_scene->SetActiveCamera(cam);

	// --- Cube ---
	auto& cube = root.AddChild<scene::ModelNode>("cube");
	cube.mesh = std::make_shared<gr::Mesh>(gr::Mesh::CreateCube());
	cube.material = std::make_shared<gr::Material>();
	cube.material->albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");
	cube.material->albedoColor = glm::vec3(0.8f, 0.2f, 0.2f);
	cube.material->specularColor = glm::vec3(1.0f);
	cube.material->ambientColor = glm::vec3(0.05f);
	cube.material->shininess = 32.0f;
	// Cube spins a little
	cube.transform.rotation = glm::angleAxis(glm::radians(25.0f), glm::vec3(0, 1, 0));

	// --- Sphere ---
	auto& sphere = root.AddChild<scene::ModelNode>("sphere");
	sphere.mesh = std::make_shared<gr::Mesh>(gr::Mesh::CreateSphere(32,32));
	sphere.material = std::make_shared<gr::Material>();
	sphere.material->albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");
	sphere.material->albedoColor = glm::vec3(0.8f, 0.2f, 0.2f);
	sphere.material->specularColor = glm::vec3(1.0f);
	sphere.material->ambientColor = glm::vec3(0.05f);
	sphere.material->shininess = 32.0f;

	sphere.transform.position = glm::vec3(5.0f, 0.0f, 0.0f);


	// --- Directional light (shines along local -Y: (0,-1,0) by default) ---
	auto& sun = root.AddChild<scene::LightNode>("sun");
	sun.lightType = scene::LightNode::LightType::Directional;
	sun.color = glm::vec3(1.0f, 0.95f, 0.85f);
	sun.intensity = 1.2f;
	sun.castShadow = false;
	// Rotate so light comes from upper-right (local (0,-1,0) → world normalize(1,-1,0))
	sun.transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));

	// Disable shadows / instancing for the minimal example
	g_scene->enableShadows = false;
	g_scene->enableInstancing = false;

	return true;
}
//=============================================================================
void GameClose()
{
	g_scene.reset();
	g_mouseLook.Reset();
	program.reset();
	mesh.Close();
}
//=============================================================================
void GameUpdate()
{
	// Input
	const float speed = 10.0f * app::GetDeltaTime();
	if (input::IsKeyDown(KeyboardType::KEY_W)) camera.Move(gr::Movement::Forward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_S)) camera.Move(gr::Movement::Backward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_A)) camera.Move(gr::Movement::Left, speed);
	if (input::IsKeyDown(KeyboardType::KEY_D)) camera.Move(gr::Movement::Right, speed);
	if (input::IsKeyDown(KeyboardType::KEY_Q)) camera.Move(gr::Movement::Down, speed);
	if (input::IsKeyDown(KeyboardType::KEY_E)) camera.Move(gr::Movement::Up, speed);

	static math::point2 prevMouse;
	static bool mouseCapture = false;

	if (input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
		g_mouseLook.OnRightDown();
	else
		g_mouseLook.OnRightUp();
	g_mouseLook.Update(camera);

	proj = glm::perspective(glm::radians(65.f), window::GetAspectRatio(), 0.1f, 1000.f);
	view = camera.GetViewMatrix();
	model = glm::mat4(1.0f);
	normalMatrix = glm::mat4(1.0f);

	gpu::uniform::BindUniform(model);
	gpu::uniform::BindUniform(view);
	gpu::uniform::BindUniform(proj);
	gpu::uniform::BindUniform(normalMatrix);

	// Update camera aspect ratio
	if (g_scene->activeCamera)
		g_scene->activeCamera->aspectRatio = window::GetAspectRatio();

	// Animate: spin cube slowly
	auto* cube = g_scene->root->FindChild("cube");
	/*if (cube)
		cube->transform.rotation *= glm::angleAxis(app::GetDeltaTime() * 5.5f, glm::vec3(0, 1, 0));*/

	// Traverse scene graph → update world matrices, collect lights
	g_scene->Update();

}
//=============================================================================
void GameFixedUpdate()
{
}
//=============================================================================
void GameRender()
{
	gpu::cmd::SetState(depthState);

	// 1. Clear
	glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 2. Viewport for the main framebuffer
	gpu::Viewport vp;
	vp.drawRect.offset = { 0, 0 };
	vp.drawRect.extent = { window::GetWidth(), window::GetHeight() };
	gpu::cmd::SetViewport(vp);

	// 3. Build render queue (no frustum culling for simplicity)
	//g_scene->enableFrustumCulling = false;
	//math::Frustum dummy; // unused when culling is disabled
	//auto queue = g_scene->BuildRenderQueue(dummy, scene::RenderPassType::Opaque);

	math::Frustum frustum;
	if (g_scene->activeCamera)
		frustum = g_scene->activeCamera->ExtractFrustum();
	auto queue = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Opaque);
	
	// 4. Render opaque objects
	g_scene->RenderOpaquePass(queue, program);


	//gpu::cmd::BindShaderProgram(program);
	//material.Bind(program);
	//mesh.Bind();
	//mesh.Draw();
}
//=============================================================================
void GameRenderUI()
{
	ImGui::Begin("Hello, world!");
	ImGui::Text("1 cube + 1 directional light");
	ImGui::Text("Draw calls: %u", g_scene->lastFrameStats.drawCalls);
	ImGui::Text("Instanced:  %u", g_scene->lastFrameStats.instancedBatches);
	ImGui::Text("Culled:     %u", g_scene->lastFrameStats.culledObjects);
	ImGui::End();
}
//=============================================================================
void GameApp()
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