#include "stdafx.h"
//=============================================================================
namespace
{
	// --- шейдер куба (Phong + текстура) ---
	// Для прохода отражения model = reflectionMatrix * cubeModel,
	// поэтому v_worldPos/v_normal уже в отражённом пространстве.
	// В основном проходе model = cubeModel, v_worldPos — реальная позиция.
	const char* cubeVertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;

void main()
{
	vec4 worldPos = u_model * vec4(a_position, 1.0);
	v_worldPos = worldPos.xyz;
	v_normal = normalize(inverse(transpose(mat3(u_model))) * a_normal);
	v_uv = a_texcoord;

	gl_Position = u_projection * u_view * worldPos;
}
)";

	const char* cubeFragmentSource = R"(
#version 460 core

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

layout(binding = 0) uniform sampler2D u_texture;

uniform vec3 u_lightDir;
uniform vec3 u_viewPos;

layout(location = 0) out vec4 o_color;

void main()
{
	vec3 N = normalize(v_normal);
	vec3 L = normalize(u_lightDir);
	vec3 V = normalize(u_viewPos - v_worldPos);
	vec3 H = normalize(L + V);

	float diff = max(dot(N, L), 0.0);
	float spec = pow(max(dot(N, H), 0.0), 32.0);

	vec4 texColor = texture(u_texture, v_uv);

	vec4 ambient  = vec4(0.15, 0.15, 0.15, 1.0);
	vec4 diffuse  = vec4(vec3(diff), 1.0);
	vec4 specular = vec4(vec3(spec), 1.0);

	o_color = texColor * (ambient + diffuse) + specular;
}
)";

	// --- шейдер плоскости (проекция текстуры отражения) ---
	// Отражение рендерится с той же камеры, но с отражённой model-матрицей куба.
	// Плоскость проецирует текстуру через обычную VP, получая screen-space маппинг.
	const char* planeVertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

layout(location = 0) out vec3 v_worldPos;

void main()
{
	vec4 worldPos = u_model * vec4(a_position, 1.0);
	v_worldPos = worldPos.xyz;

	gl_Position = u_projection * u_view * worldPos;
}
)";

	const char* planeFragmentSource = R"(
#version 460 core

layout(location = 0) in vec3 v_worldPos;

uniform sampler2D u_reflectionTex;
uniform mat4 u_reflectionVP;

layout(location = 0) out vec4 o_color;

void main()
{
	// Проецируем мировую позицию в screen-space той же камеры,
	// которой рендерилась текстура отражения
	vec4 projPos = u_reflectionVP * vec4(v_worldPos, 1.0);
	vec3 ndc = projPos.xyz / projPos.w;

	vec2 uv = ndc.xy * 0.5 + 0.5;
	uv.y = 1.0 - uv.y;

	vec4 reflection = texture(u_reflectionTex, uv);

	// Затемняем отражение, имитируя потери света
	o_color = vec4(reflection.rgb * 0.65, 1.0);
}
)";

	// --- ресурсы ---
	gpu::program::ShaderProgramPtr cubeProgram;
	gpu::program::ShaderProgramPtr planeProgram;

	gr::Mesh cubeMesh;
	gr::Mesh planeMesh;

	gpu::texture::TexturePtr cubeTexture;
	gpu::texture::SamplerPtr defaultSampler;

	// FBO для прохода отражения
	gpu::texture::TexturePtr reflectionColorTex;
	gpu::texture::TexturePtr reflectionDepthTex;
	gpu::fbo::FramebufferPtr reflectionFbo;
	gpu::texture::SamplerPtr reflectionSampler;

	gpu::DepthState depthState;
	gpu::RasterizationState defaultRasterState;
	gpu::RasterizationState reflectionRasterState;

	gr::Camera camera(
		glm::vec3(0.0f, 2.5f, -5.0f),
		glm::vec3(0.0f, 0.5f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));
	input::MouseLook mouseLook;

	constexpr float PLANE_SIZE = 8.0f;
	constexpr uint32_t REFLECTION_SIZE = 1024u;
}
//=============================================================================
static bool GameInit()
{
	// --- программа куба ---
	cubeProgram = gpu::program::CreateShaderProgram({
		.name = "CubeProgram",
		.vertexShaderCode = cubeVertexSource,
		.fragmentShaderCode = cubeFragmentSource });

	// --- программа плоскости ---
	planeProgram = gpu::program::CreateShaderProgram({
		.name = "PlaneProgram",
		.vertexShaderCode = planeVertexSource,
		.fragmentShaderCode = planeFragmentSource });

	// --- меши ---
	cubeMesh = gr::Mesh::CreateCube();
	planeMesh = gr::Mesh::CreatePlane(PLANE_SIZE);

	// --- текстура ---
	cubeTexture = gpu::texture::LoadTexture2D("data/textures/uv.png");

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
	ss.addressModeU = gpu::AddressMode::Repeat;
	ss.addressModeV = gpu::AddressMode::Repeat;
	defaultSampler = gpu::texture::CreateSampler(ss);

	// --- FBO для отражения (цвет + глубина) ---
	reflectionColorTex = gpu::texture::CreateTexture2D(
		{ REFLECTION_SIZE, REFLECTION_SIZE },
		gpu::Format::R8G8B8A8_UNORM,
		"ReflectionColor");

	reflectionDepthTex = gpu::texture::CreateTexture2D(
		{ REFLECTION_SIZE, REFLECTION_SIZE },
		gpu::Format::D32_FLOAT,
		"ReflectionDepth");

	reflectionFbo = gpu::fbo::CreateFramebuffer({
		.colorAttachments = { {
			.texture = reflectionColorTex,
			.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
			.clearValue = {0.0f, 0.0f, 0.0f, 1.0f},
		} },
		.depthAttachment = { {
			.texture = reflectionDepthTex,
			.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
			.clearValue = {1.0f, 0},
		} },
	});

	gpu::texture::SamplerState refSs;
	refSs.minFilter = gpu::Filter::Linear;
	refSs.magFilter = gpu::Filter::Linear;
	refSs.addressModeU = gpu::AddressMode::ClampToEdge;
	refSs.addressModeV = gpu::AddressMode::ClampToEdge;
	reflectionSampler = gpu::texture::CreateSampler(refSs);

	// --- состояния ---
	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;

	reflectionRasterState.cullMode = gpu::CullMode::Front;

	return true;
}
//=============================================================================
static void GameClose()
{
	mouseLook.Reset();
	cubeProgram.reset();
	planeProgram.reset();
	cubeMesh.Close();
	planeMesh.Close();
	cubeTexture.reset();
	defaultSampler.reset();
	reflectionColorTex.reset();
	reflectionDepthTex.reset();
	reflectionFbo.reset();
	reflectionSampler.reset();
}
//=============================================================================
static void GameUpdate()
{
	const float speed = 10.0f * app::GetDeltaTime();
	if (input::IsKeyDown(KeyboardType::KEY_W)) camera.Move(gr::Movement::Forward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_S)) camera.Move(gr::Movement::Backward, speed);
	if (input::IsKeyDown(KeyboardType::KEY_A)) camera.Move(gr::Movement::Left, speed);
	if (input::IsKeyDown(KeyboardType::KEY_D)) camera.Move(gr::Movement::Right, speed);
	if (input::IsKeyDown(KeyboardType::KEY_Q)) camera.Move(gr::Movement::Down, speed);
	if (input::IsKeyDown(KeyboardType::KEY_E)) camera.Move(gr::Movement::Up, speed);

	if (input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
		mouseLook.OnRightDown();
	else
		mouseLook.OnRightUp();
	mouseLook.Update(camera);
}
//=============================================================================
static void GameFixedUpdate()
{}
//=============================================================================
static void GameRender()
{
	const glm::mat4 projMatrix = glm::perspective(
		glm::radians(65.f), window::GetAspectRatio(), 0.1f, 1000.f);
	const glm::mat4 viewMatrix = camera.GetViewMatrix();

	// Матрица отражения относительно плоскости Y=0
	const glm::mat4 reflectionMatrix = glm::scale(
		glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));

	// Кубик стоит на плоскости (центр в y=0.5, нижняя грань на y=0)
	const glm::mat4 cubeModelMat = glm::translate(
		glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));

	const glm::vec3 lightDir = glm::normalize(glm::vec3(1.0f, 2.0f, 1.0f));

	// === Pass 1: отражение в FBO ===
	// Рендерим куб с той же камеры, но model = reflectionMatrix * cubeModel,
	// чтобы куб оказался в отражённой позиции (y = -0.5).
	// Это даёт правильные v_worldPos/v_normal во фрагментном шейдере для освещения.
	{
		const float reflAspect = static_cast<float>(REFLECTION_SIZE) /
			static_cast<float>(REFLECTION_SIZE);
		const glm::mat4 reflProj = glm::perspective(
			glm::radians(65.f), reflAspect, 0.1f, 1000.f);

		gpu::cmd::BeginDraw(reflectionFbo, "ReflectionPass");
		gpu::cmd::SetViewport({
			.drawRect = {0, 0, REFLECTION_SIZE, REFLECTION_SIZE} });

		gpu::cmd::SetState(depthState);
		gpu::cmd::SetState(reflectionRasterState);

		gpu::cmd::BindShaderProgram(cubeProgram);
		gpu::cmd::BindSampledImage(0, cubeTexture, defaultSampler);

		// Отражаем model-матрицу: куб уходит под плоскость
		const glm::mat4 reflModel = reflectionMatrix * cubeModelMat;

		// Свет тоже отражаем
		const glm::vec3 reflLightDir = glm::normalize(
			glm::vec3(lightDir.x, -lightDir.y, lightDir.z));

		// Позиция камеры в отражённом мире
		glm::vec3 reflViewPos = camera.GetPosition();
		reflViewPos.y = -reflViewPos.y;

		gpu::program::SetUniform(cubeProgram,
			gpu::program::GetUniformLocation(cubeProgram, "u_projection"),
			reflProj);
		gpu::program::SetUniform(cubeProgram,
			gpu::program::GetUniformLocation(cubeProgram, "u_view"),
			viewMatrix);
		gpu::program::SetUniform(cubeProgram,
			gpu::program::GetUniformLocation(cubeProgram, "u_model"),
			reflModel);
		gpu::program::SetUniform(cubeProgram,
			gpu::program::GetUniformLocation(cubeProgram, "u_lightDir"),
			reflLightDir);
		gpu::program::SetUniform(cubeProgram,
			gpu::program::GetUniformLocation(cubeProgram, "u_viewPos"),
			reflViewPos);

		cubeMesh.Bind();
		cubeMesh.Draw();

		gpu::cmd::EndDraw();
	}

	// === Pass 2: основной рендер в swapchain ===
	{
		gpu::fbo::SwapchainRenderInfo swapchainRI = {};
		swapchainRI.colorLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
		swapchainRI.clearColorValue[0] = 0.12f;
		swapchainRI.clearColorValue[1] = 0.32f;
		swapchainRI.clearColorValue[2] = 0.88f;
		swapchainRI.clearColorValue[3] = 1.0f;
		swapchainRI.depthLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
		swapchainRI.viewport.drawRect.offset = { 0, 0 };
		swapchainRI.viewport.drawRect.extent = { window::GetWidth(), window::GetHeight() };
		gpu::cmd::BeginDraw(swapchainRI, "MainFrame");
		{
			// --- плоскость с отражением ---
			gpu::cmd::SetState(depthState);
			gpu::cmd::SetState(defaultRasterState);
			gpu::cmd::BindShaderProgram(planeProgram);
			gpu::cmd::BindSampledImage(0, reflectionColorTex, reflectionSampler);

			// u_reflectionVP = обычная VP (без отражения), т.к. текстура уже
			// содержит отражённый куб в screen-space той же камеры
			gpu::program::SetUniform(planeProgram,
				gpu::program::GetUniformLocation(planeProgram, "u_projection"),
				projMatrix);
			gpu::program::SetUniform(planeProgram,
				gpu::program::GetUniformLocation(planeProgram, "u_view"),
				viewMatrix);
			gpu::program::SetUniform(planeProgram,
				gpu::program::GetUniformLocation(planeProgram, "u_model"),
				glm::mat4(1.0f));
			gpu::program::SetUniform(planeProgram,
				gpu::program::GetUniformLocation(planeProgram, "u_reflectionVP"),
				projMatrix * viewMatrix);

			planeMesh.Bind();
			planeMesh.Draw();

			// --- куб (обычный) ---
			gpu::cmd::BindShaderProgram(cubeProgram);
			gpu::cmd::BindSampledImage(0, cubeTexture, defaultSampler);

			gpu::program::SetUniform(cubeProgram,
				gpu::program::GetUniformLocation(cubeProgram, "u_projection"),
				projMatrix);
			gpu::program::SetUniform(cubeProgram,
				gpu::program::GetUniformLocation(cubeProgram, "u_view"),
				viewMatrix);
			gpu::program::SetUniform(cubeProgram,
				gpu::program::GetUniformLocation(cubeProgram, "u_model"),
				cubeModelMat);
			gpu::program::SetUniform(cubeProgram,
				gpu::program::GetUniformLocation(cubeProgram, "u_lightDir"),
				lightDir);
			gpu::program::SetUniform(cubeProgram,
				gpu::program::GetUniformLocation(cubeProgram, "u_viewPos"),
				camera.GetPosition());

			cubeMesh.Bind();
			cubeMesh.Draw();
		}
		gpu::cmd::EndDraw();
	}
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Planar Reflection");
	ImGui::Text("WASD + Right-click mouse look");
	ImGui::Text("Plane reflects the cube above it");
	ImGui::Text("Resolution: %u x %u", REFLECTION_SIZE, REFLECTION_SIZE);
	ImGui::End();
}
//=============================================================================
void gpu004_reflection()
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
