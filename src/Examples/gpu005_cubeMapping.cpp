#include "stdafx.h"
//=============================================================================
namespace
{
	const char* vertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_vertex;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_projectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;
uniform mat3 u_inverseViewMatrix;

layout(location = 0) out vec3 v_reflect;

void main()
{
	vec4 vertex = u_modelViewMatrix * vec4(a_vertex, 1.0);

	vec3 normal = u_normalMatrix * a_normal;

	vec3 incident = vec3(vertex);

	vec3 reflectView = reflect(incident, normal);

	v_reflect = u_inverseViewMatrix * reflectView;

	gl_Position = u_projectionMatrix * vertex;
}
)";

	const char* fragmentSource = R"(
#version 460 core

layout(binding = 0) uniform samplerCube u_cubemapTexture;

layout(location = 0) in vec3 v_reflect;

layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = texture(u_cubemapTexture, v_reflect);
}
)";

	gpu::program::ShaderProgramPtr program;

	gpu::texture::TexturePtr cubemapTexture;
	gpu::texture::SamplerPtr sampler;

	int g_projectionMatrixLocation  = -1;
	int g_modelViewMatrixLocation   = -1;
	int g_normalMatrixLocation      = -1;
	int g_inverseViewMatrixLocation = -1;

	glm::mat4 g_viewMatrix(1.0f);
	glm::mat3 g_invViewMatrix(1.0f);

	gr::Mesh g_cube;

	gpu::DepthState depthState;
	gpu::RasterizationState defaultRasterState;
}
//=============================================================================
static bool GameInit()
{
	gpu::program::GraphicsProgramCreateInfo createInfo{
		.name = "CubeMapping",
		.vertexShaderCode = vertexSource,
		.fragmentShaderCode = fragmentSource };
	program = gpu::program::CreateShaderProgram(createInfo);

	g_projectionMatrixLocation  = gpu::program::GetUniformLocation(program, "u_projectionMatrix");
	g_modelViewMatrixLocation   = gpu::program::GetUniformLocation(program, "u_modelViewMatrix");
	g_normalMatrixLocation      = gpu::program::GetUniformLocation(program, "u_normalMatrix");
	g_inverseViewMatrixLocation = gpu::program::GetUniformLocation(program, "u_inverseViewMatrix");

	// Camera: look from (0, 0, 5) at origin
	g_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	// Inverse view matrix (rotation only): for orthogonal matrices, transpose = inverse
	g_invViewMatrix = glm::transpose(glm::mat3(g_viewMatrix));

	g_cube = gr::Mesh::CreateCube();

	// Load cubemap from 6 TGA faces
	static constexpr const char* cubemapFiles[] = {
		"data/textures/cubemap1/cm_pos_x.tga",
		"data/textures/cubemap1/cm_neg_x.tga",
		"data/textures/cubemap1/cm_pos_y.tga",
		"data/textures/cubemap1/cm_neg_y.tga",
		"data/textures/cubemap1/cm_pos_z.tga",
		"data/textures/cubemap1/cm_neg_z.tga",
	};

	int imgW = 0, imgH = 0;
	stbi_set_flip_vertically_on_load(false);
	auto firstPixels = stbi_load(cubemapFiles[0], &imgW, &imgH, nullptr, 4);
	assert(firstPixels && "Failed to load cubemap face 0");

	gpu::texture::TextureCreateInfo cubemapInfo{
		.imageType = gpu::ImageType::TextureCubemap,
		.format = gpu::Format::R8G8B8A8_UNORM,
		.extent = {static_cast<uint32_t>(imgW), static_cast<uint32_t>(imgH), 1},
		.mipLevels = 1,
		.arrayLayers = 6,
		.sampleCount = gpu::SampleCount::Samples1,
	};
	cubemapTexture = gpu::texture::CreateTexture(cubemapInfo, "cubemap");

	gpu::texture::UpdateImage(cubemapTexture, {
		.level = 0, .offset = {0, 0, 0},
		.extent = {static_cast<uint32_t>(imgW), static_cast<uint32_t>(imgH), 1},
		.format = gpu::UploadFormat::RGBA, .type = gpu::UploadType::UBYTE,
		.pixels = firstPixels,
		});
	stbi_image_free(firstPixels);

	for (unsigned i = 1; i < 6; ++i)
	{
		int w = 0, h = 0;
		auto pixels = stbi_load(cubemapFiles[i], &w, &h, nullptr, 4);
		assert(pixels && w == imgW && h == imgH && "Cubemap face size mismatch");
		gpu::texture::UpdateImage(cubemapTexture, {
			.level = 0, .offset = {0, 0, i},
			.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1},
			.format = gpu::UploadFormat::RGBA, .type = gpu::UploadType::UBYTE,
			.pixels = pixels,
			});
		stbi_image_free(pixels);
	}

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Linear;
	ss.magFilter = gpu::Filter::Linear;
	ss.addressModeU = gpu::AddressMode::ClampToEdge;
	ss.addressModeV = gpu::AddressMode::ClampToEdge;
	ss.addressModeW = gpu::AddressMode::ClampToEdge;
	sampler = gpu::texture::CreateSampler(ss);

	gpu::program::BindShaderProgram(program);

	// Set constant uniforms
	gpu::program::SetUniform(program, g_inverseViewMatrixLocation, g_invViewMatrix);

	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;

	return true;
}
//=============================================================================
static void GameClose()
{
	program.reset();
	g_cube.Close();
	cubemapTexture.reset();
	sampler.reset();
}
//=============================================================================
static void GameUpdate()
{
	static float angle = 0.0f;
	float deltaTime = app::GetDeltaTime();

	angle += 20.0f * deltaTime;

	// Model: Ry(angle) * Rx(15 deg) — matches original GLUS RzRxRy order
	glm::mat4 model(1.0f);
	model = glm::rotate(model, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::rotate(model, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat4 modelView = g_viewMatrix * model;
	glm::mat3 normalMatrix = glm::mat3(modelView);

	gpu::program::SetUniform(program, g_modelViewMatrixLocation, modelView);
	gpu::program::SetUniform(program, g_normalMatrixLocation, normalMatrix);
	gpu::program::SetUniform(program, g_projectionMatrixLocation,
		glm::perspective(glm::radians(40.0f), window::GetAspectRatio(), 1.0f, 100.0f));
}
//=============================================================================
static void GameFixedUpdate()
{}
//=============================================================================
static void GameRender()
{
	gpu::fbo::SwapchainRenderInfo swapchainRI = {};
	swapchainRI.colorLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.clearColorValue[0] = 0.0f;
	swapchainRI.clearColorValue[1] = 0.0f;
	swapchainRI.clearColorValue[2] = 0.0f;
	swapchainRI.clearColorValue[3] = 1.0f;
	swapchainRI.depthLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.viewport.drawRect.offset = { 0, 0 };
	swapchainRI.viewport.drawRect.extent = { window::GetWidth(), window::GetHeight() };
	gpu::cmd::BeginDraw(swapchainRI, "MainFrame");
	{
		gpu::cmd::SetState(depthState);
		gpu::cmd::SetState(defaultRasterState);
		gpu::cmd::BindShaderProgram(program);

		gpu::cmd::BindSampledImage(0, cubemapTexture, sampler);
		g_cube.Bind();
		g_cube.Draw();
	}
	gpu::cmd::EndDraw();
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Cube Mapping");
	ImGui::Text("Example08: Environment Reflection");
	ImGui::Text("Rotating reflective cube with cubemap");
	ImGui::End();
}
//=============================================================================
void gpu005_cubeMapping()
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
