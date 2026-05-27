#include "stdafx.h"
//=============================================================================
namespace
{
	const char* vertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;

uniform mat4 proj;
uniform mat4 view;
uniform mat4 model;

layout(location = 0) out vec3 v_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec2 v_uv;

void main()
{
	v_position = a_pos;
	v_normal = normalize(inverse(transpose(mat3(model))) * a_normal);
	v_uv = a_uv;

	gl_Position = proj * view * model * vec4(a_pos, 1.0);
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

	struct VertexPNT final
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
	};

	auto VertexPNTBindingDescs = std::vector{
		gpu::vao::VertexInputBindingDescription{
			// position
			.location = 0,
			.binding = 0,
			.format = gpu::Format::R32G32B32_FLOAT,
			.offset = offsetof(VertexPNT, position),
		},
		gpu::vao::VertexInputBindingDescription{
			// normal
			.location = 1,
			.binding = 0,
			.format = gpu::Format::R32G32B32_FLOAT,
			.offset = offsetof(VertexPNT, normal),
		},
		gpu::vao::VertexInputBindingDescription{
			// texcoord
			.location = 2,
			.binding = 0,
			.format = gpu::Format::R32G32_FLOAT,
			.offset = offsetof(VertexPNT, uv),
		},
	};

	static constexpr auto gCubeVertices = std::array<VertexPNT, 24>{
		// front (+z)
		VertexPNT{{-0.5, -0.5, 0.5}, {0, 0, 1}, {0, 0}},
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

	gpu::program::ShaderProgramPtr program;
	gpu::vao::VertexArrayPtr vao;
	gpu::buffer::BufferPtr vbo;
	gpu::buffer::BufferPtr ibo;

	gpu::texture::TexturePtr texture;
	gpu::texture::SamplerPtr sampler;

	gpu::uniform::Uniform<glm::mat4> proj;
	gpu::uniform::Uniform<glm::mat4> view;
	gpu::uniform::Uniform<glm::mat4> model;
}
//=============================================================================
bool GameInit()
{
	gpu::program::GraphicsProgramCreateInfo createInfo{
		.name               = "Program",
		.vertexShaderCode   = vertexSource,
		.fragmentShaderCode = fragmentSource };
	program = gpu::program::CreateShaderProgram(createInfo);

	gpu::uniform::InitUniform(proj, program, "proj");
	gpu::uniform::InitUniform(view, program, "view");
	gpu::uniform::InitUniform(model, program, "model");

	vao = gpu::vao::CreateVertexArray(VertexPNTBindingDescs);

	vbo = gpu::buffer::CreateBuffer(gCubeVertices);
	ibo = gpu::buffer::CreateBuffer(gCubeIndices);

	texture = gpu::texture::LoadTexture2D("data/textures/bluenoise32.png");

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
	ss.addressModeU = gpu::AddressMode::Repeat;
	ss.addressModeV = gpu::AddressMode::Repeat;
	sampler = gpu::texture::CreateSampler(ss);

	gpu::SetCapability(gpu::RenderingCapability::DepthTest, true);

	return true;
}
//=============================================================================
void GameClose()
{
	program.reset();
	vao.reset();
	vbo.reset();
}
//=============================================================================
void GameUpdate()
{
	proj = glm::perspective(glm::radians(65.f), window::GetAspectRatio(), 0.1f, 5.f);
	view = glm::lookAt(glm::vec3(0.0f, 1.2f, -3.0f), glm::vec3(0.0f), glm::vec3(0, 1, 0));
	model = glm::mat4(1.0f);

	gpu::uniform::BindUniform(proj);
	gpu::uniform::BindUniform(view);
	gpu::uniform::BindUniform(model);
}
//=============================================================================
void GameFixedUpdate()
{
}
//=============================================================================
void GameRender()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	gpu::cmd::BindShaderProgram(program);
	gpu::cmd::BindSampledImage(0, texture, sampler);

	gpu::cmd::BindVertexArray(vao);
	gpu::cmd::BindVertexBuffer(vao, 0, vbo, 0, sizeof(VertexPNT));
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