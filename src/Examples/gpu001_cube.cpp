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

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;

void main()
{
	v_position = a_position;
	v_normal = normalize(inverse(transpose(mat3(u_model))) * a_normal);
	v_uv = a_texcoord;

	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
)";

	const char* fragmentSource = R"(
#version 460 core

layout(location = 0) in vec3 v_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_uv;

layout(binding = 0) uniform sampler2D diffuseTex;

layout(location = 0) out vec4 o_color;

void main()
{
	o_color = texture(diffuseTex, v_uv);
}
)";

	const char* skyboxVertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_position;

uniform mat4 u_projection;
uniform mat4 u_view;

layout(location = 0) out vec3 v_worldPos;

void main()
{
	v_worldPos = a_position;
	vec4 clipPos = u_projection * mat4(mat3(u_view)) * vec4(a_position, 1.0);
	gl_Position = clipPos.xyww;
}
)";

	const char* skyboxFragmentSource = R"(
#version 460 core

layout(location = 0) in vec3 v_worldPos;

layout(binding = 0) uniform samplerCube u_skybox;

layout(location = 0) out vec4 o_color;

void main()
{
	o_color = texture(u_skybox, v_worldPos);
}
)";


	static constexpr auto gCubeVertices = std::array<gr::MeshVertex, 24>{
		// front (+z)
		gr::MeshVertex{{-0.5, -0.5, 0.5}, {0, 0, 1}, {0, 0}},
		{{0.5, -0.5, 0.5}, {0, 0, 1}, {1, 0}},
		{{0.5, 0.5, 0.5}, {0, 0, 1}, {1, 1}},
		{{-0.5, 0.5, 0.5}, {0, 0, 1}, {0, 1}},

		// back (-z)
		{{-0.5, 0.5, -0.5}, {0, 0, -1}, {1, 1}},
		{{0.5, 0.5, -0.5}, {0, 0, -1}, {0, 1}},
		{{0.5, -0.5, -0.5}, {0, 0, -1}, {0, 0}},
		{{-0.5, -0.5, -0.5}, {0, 0, -1}, {1, 0}},

		// left (-x)
		{{-0.5, -0.5, -0.5}, {-1, 0, 0}, {0, 0}},
		{{-0.5, -0.5, 0.5}, {-1, 0, 0}, {1, 0}},
		{{-0.5, 0.5, 0.5}, {-1, 0, 0}, {1, 1}},
		{{-0.5, 0.5, -0.5}, {-1, 0, 0}, {0, 1}},

		// right (+x)
		{{0.5, 0.5, -0.5}, {1, 0, 0}, {1, 1}},
		{{0.5, 0.5, 0.5}, {1, 0, 0}, {0, 1}},
		{{0.5, -0.5, 0.5}, {1, 0, 0}, {0, 0}},
		{{0.5, -0.5, -0.5}, {1, 0, 0}, {1, 0}},

		// top (+y)
		{{-0.5, 0.5, 0.5}, {0, 1, 0}, {0, 0}},
		{{0.5, 0.5, 0.5}, {0, 1, 0}, {1, 0}},
		{{0.5, 0.5, -0.5}, {0, 1, 0}, {1, 1}},
		{{-0.5, 0.5, -0.5}, {0, 1, 0}, {0, 1}},

		// bottom (-y)
		{{-0.5, -0.5, -0.5}, {0, -1, 0}, {0, 0}},
		{{0.5, -0.5, -0.5}, {0, -1, 0}, {1, 0}},
		{{0.5, -0.5, 0.5}, {0, -1, 0}, {1, 1}},
		{{-0.5, -0.5, 0.5}, {0, -1, 0}, {0, 1}},
	};

	static constexpr auto gCubeIndices = std::array<uint32_t, 36>{
	  0,  1,  2,  2,  3,  0,
	  4,  5,  6,  6,  7,  4,
	  8,  9,  10, 10, 11, 8,
	  12, 13, 14, 14, 15, 12,
	  16, 17, 18, 18, 19, 16,
	  20, 21, 22, 22, 23, 20,
	};

	static constexpr auto gSkyboxVertices = std::array<glm::vec3, 36>{
		// front (+z)
		glm::vec3{-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1},
		{ 1,  1,  1}, {-1,  1,  1}, {-1, -1,  1},
		// back (-z)
		{-1, -1, -1}, {-1,  1, -1}, { 1,  1, -1},
		{ 1,  1, -1}, { 1, -1, -1}, {-1, -1, -1},
		// left (-x)
		{-1,  1,  1}, {-1,  1, -1}, {-1, -1, -1},
		{-1, -1, -1}, {-1, -1,  1}, {-1,  1,  1},
		// right (+x)
		{ 1,  1,  1}, { 1, -1,  1}, { 1, -1, -1},
		{ 1, -1, -1}, { 1,  1, -1}, { 1,  1,  1},
		// top (+y)
		{-1,  1, -1}, {-1,  1,  1}, { 1,  1,  1},
		{ 1,  1,  1}, { 1,  1, -1}, {-1,  1, -1},
		// bottom (-y)
		{-1, -1, -1}, { 1, -1, -1}, { 1, -1,  1},
		{ 1, -1,  1}, {-1, -1,  1}, {-1, -1, -1},
	};

	gpu::program::ShaderProgramPtr program;
	gpu::vao::VertexArrayPtr vao;
	gpu::buffer::BufferPtr vbo;
	gpu::buffer::BufferPtr ibo;
	
	gpu::texture::TexturePtr texture;
	gpu::texture::SamplerPtr sampler;

	// Skybox
	gpu::program::ShaderProgramPtr skyboxProgram;
	gpu::vao::VertexArrayPtr skyboxVao;
	gpu::buffer::BufferPtr skyboxVbo;
	gpu::texture::TexturePtr skyboxTexture;
	gpu::texture::SamplerPtr skyboxSampler;
	gpu::uniform::Uniform<glm::mat4> skyboxProj;
	gpu::uniform::Uniform<glm::mat4> skyboxView;
	gpu::RasterizationState defaultRasterState;
	gpu::RasterizationState skyboxRasterState;
	gpu::DepthState skyboxDepthState;

	gpu::uniform::Uniform<glm::mat4> proj;
	gpu::uniform::Uniform<glm::mat4> view;
	gpu::uniform::Uniform<glm::mat4> model;

	gpu::DepthState depthState;

	gr::Camera camera(glm::vec3(0.0f, 1.2f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	input::MouseLook mouseLook;
}
//=============================================================================
bool GameInit()
{
	gpu::program::GraphicsProgramCreateInfo createInfo{
		.name = "Program",
		.vertexShaderCode = vertexSource,
		.fragmentShaderCode = fragmentSource };
	program = gpu::program::CreateShaderProgram(createInfo);

	gpu::uniform::InitUniform(proj, program, "u_projection");
	gpu::uniform::InitUniform(view, program, "u_view");
	gpu::uniform::InitUniform(model, program, "u_model");

	// Skybox
	gpu::program::GraphicsProgramCreateInfo skyboxInfo{
		.name = "SkyboxProgram",
		.vertexShaderCode = skyboxVertexSource,
		.fragmentShaderCode = skyboxFragmentSource };
	skyboxProgram = gpu::program::CreateShaderProgram(skyboxInfo);
	gpu::uniform::InitUniform(skyboxProj, skyboxProgram, "u_projection");
	gpu::uniform::InitUniform(skyboxView, skyboxProgram, "u_view");

	static const std::vector skyboxBindingDescs = {
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding = 0,
			.format = gpu::Format::R32G32B32_FLOAT,
			.offset = 0,
		},
	};
	skyboxVao = gpu::vao::CreateVertexArray(skyboxBindingDescs);
	skyboxVbo = gpu::buffer::CreateBuffer(gSkyboxVertices);

	static constexpr const char* skyboxFiles[] = {
		"data/textures/skybox1/LeftImage.png",
		"data/textures/skybox1/RightImage.png",
		"data/textures/skybox1/TopImage.png",
		"data/textures/skybox1/BottomImage.png",
		"data/textures/skybox1/FrontImage.png",
		"data/textures/skybox1/BackImage.png",
	};
	int imgW = 0, imgH = 0;
	stbi_set_flip_vertically_on_load(true);
	auto firstPixels = stbi_load(skyboxFiles[0], &imgW, &imgH, nullptr, 4);
	assert(firstPixels);
	gpu::texture::TextureCreateInfo cubemapInfo{
		.imageType = gpu::ImageType::TextureCubemap,
		.format = gpu::Format::R8G8B8A8_UNORM,
		.extent = {static_cast<uint32_t>(imgW), static_cast<uint32_t>(imgH), 1},
		.mipLevels = 1,
		.arrayLayers = 6,
		.sampleCount = gpu::SampleCount::Samples1,
	};
	skyboxTexture = gpu::texture::CreateTexture(cubemapInfo, "skybox");
	gpu::texture::UpdateImage(skyboxTexture, {
		.level = 0, .offset = {0, 0, 0},
		.extent = {static_cast<uint32_t>(imgW), static_cast<uint32_t>(imgH), 1},
		.format = gpu::UploadFormat::RGBA, .type = gpu::UploadType::UBYTE,
		.pixels = firstPixels,
	});
	stbi_image_free(firstPixels);
	for (unsigned i = 1; i < 6; i++)
	{
		int w = 0, h = 0;
		auto pixels = stbi_load(skyboxFiles[i], &w, &h, nullptr, 4);
		assert(pixels && w == imgW && h == imgH);
		gpu::texture::UpdateImage(skyboxTexture, {
			.level = 0, .offset = {0, 0, i},
			.extent = {static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1},
			.format = gpu::UploadFormat::RGBA, .type = gpu::UploadType::UBYTE,
			.pixels = pixels,
		});
		stbi_image_free(pixels);
	}

	gpu::texture::SamplerState skyboxSs;
	skyboxSs.minFilter = gpu::Filter::Linear;
	skyboxSs.magFilter = gpu::Filter::Linear;
	skyboxSs.addressModeU = gpu::AddressMode::ClampToEdge;
	skyboxSs.addressModeV = gpu::AddressMode::ClampToEdge;
	skyboxSs.addressModeW = gpu::AddressMode::ClampToEdge;
	skyboxSampler = gpu::texture::CreateSampler(skyboxSs);

	skyboxDepthState.depthTestEnable = true;
	skyboxDepthState.depthWriteEnable = false;
	skyboxDepthState.depthCompareOp = gpu::CompareOp::LessEqual;
	skyboxRasterState.cullMode = gpu::CullMode::None;

	vao = gpu::vao::CreateVertexArray(gr::MeshVertexBindingDescs);

	vbo = gpu::buffer::CreateBuffer(gCubeVertices);
	ibo = gpu::buffer::CreateBuffer(gCubeIndices);

	texture = gpu::texture::LoadTexture2D("data/textures/uv.png");

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
	ss.addressModeU = gpu::AddressMode::Repeat;
	ss.addressModeV = gpu::AddressMode::Repeat;
	sampler = gpu::texture::CreateSampler(ss);

	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;

	return true;
}
//=============================================================================
void GameClose()
{
	mouseLook.Reset();
	program.reset();
	vao.reset();
	vbo.reset();
	ibo.reset();
	skyboxProgram.reset();
	skyboxVao.reset();
	skyboxVbo.reset();
	skyboxTexture.reset();
	skyboxSampler.reset();
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
		mouseLook.OnRightDown();
	else
		mouseLook.OnRightUp();
	mouseLook.Update(camera);

	proj = glm::perspective(glm::radians(65.f), window::GetAspectRatio(), 0.1f, 1000.f);
	view = camera.GetViewMatrix();
	model = glm::mat4(1.0f);

	gpu::uniform::BindUniform(proj);
	gpu::uniform::BindUniform(view);
	gpu::uniform::BindUniform(model);

	skyboxProj = proj.value;
	skyboxView = view.value;
}
//=============================================================================
void GameFixedUpdate()
{}
//=============================================================================
void GameRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Skybox
	gpu::cmd::SetState(skyboxDepthState);
	gpu::cmd::SetState(skyboxRasterState);
	gpu::cmd::BindShaderProgram(skyboxProgram);
	gpu::cmd::BindSampledImage(0, skyboxTexture, skyboxSampler);
	gpu::uniform::BindUniform(skyboxProj);
	gpu::uniform::BindUniform(skyboxView);
	gpu::cmd::BindVertexArray(skyboxVao);
	gpu::cmd::BindVertexBuffer(skyboxVao, 0, skyboxVbo, 0, sizeof(glm::vec3));
	gpu::cmd::Draw(static_cast<uint32_t>(gSkyboxVertices.size()), 1, 0, 0);

	// Cube
	gpu::cmd::SetState(defaultRasterState);
	gpu::cmd::SetState(depthState);
	gpu::cmd::BindShaderProgram(program);
	gpu::cmd::BindSampledImage(0, texture, sampler);

	gpu::cmd::BindVertexArray(vao);
	gpu::cmd::BindVertexBuffer(vao, 0, vbo, 0, sizeof(gr::MeshVertex));
	gpu::cmd::BindIndexBuffer(vao, ibo, gpu::IndexType::UnsignedInt);
	gpu::cmd::DrawIndexed(static_cast<uint32_t>(gCubeIndices.size()), 1, 0, 0, 0);
}
//=============================================================================
void GameRenderUI()
{
	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");
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