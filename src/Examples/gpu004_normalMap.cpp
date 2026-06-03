#include "stdafx.h"
//=============================================================================
namespace
{
	const char* vertexSource = R"(
#version 460 core

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_tangent;
layout(location = 2) in vec3 a_bitangent;
layout(location = 3) in vec3 a_normal;
layout(location = 4) in vec2 a_texCoord;

uniform mat4 u_projectionMatrix;
uniform mat4 u_modelViewMatrix;
uniform mat3 u_normalMatrix;
uniform vec3 u_lightDirection;

layout(location = 0) out vec2 v_texCoord;
layout(location = 1) out vec3 v_tsLight;
layout(location = 2) out vec3 v_tsEye;

void main()
{
	v_texCoord = a_texCoord;

	vec3 tangent   = u_normalMatrix * a_tangent;
	vec3 bitangent = u_normalMatrix * a_bitangent;
	vec3 normal    = u_normalMatrix * a_normal;

	v_tsLight.x = dot(tangent, u_lightDirection);
	v_tsLight.y = dot(bitangent, u_lightDirection);
	v_tsLight.z = dot(normal, u_lightDirection);

	vec4 vertex = u_modelViewMatrix * a_vertex;
	vec3 eye = -vec3(vertex);

	v_tsEye.x = dot(eye, tangent);
	v_tsEye.y = dot(eye, bitangent);
	v_tsEye.z = dot(eye, normal);

	gl_Position = u_projectionMatrix * vertex;
}
)";

	const char* fragmentSource = R"(
#version 460 core

uniform sampler2D u_texture;
uniform sampler2D u_normalMap;

layout(location = 0) in vec2 v_texCoord;
layout(location = 1) in vec3 v_tsLight;
layout(location = 2) in vec3 v_tsEye;

layout(location = 0) out vec4 fragColor;

void main()
{
	vec4 textureColor = texture(u_texture, v_texCoord);

	vec4 color = 0.3 * textureColor;

	vec3 normalDX = normalize(texture(u_normalMap, v_texCoord).xyz * 2.0 - 1.0);

	// Convert from DirectX to OpenGL tangent space (Y flip)
	vec3 normal;
	normal.x = dot(vec3(1.0, 0.0, 0.0), normalDX);
	normal.y = dot(vec3(0.0, -1.0, 0.0), normalDX);
	normal.z = dot(vec3(0.0, 0.0, 1.0), normalDX);

	vec3 light = normalize(v_tsLight);

	float nDotL = max(dot(light, normal), 0.0);

	if (nDotL > 0.0)
	{
		vec3 eye = normalize(v_tsEye);

		vec3 reflection = reflect(-light, normal);

		float eDotR = max(dot(eye, reflection), 0.0);

		color += textureColor * nDotL;

		color += vec4(0.1, 0.1, 0.1, 0.1) * pow(eDotR, 20.0);
	}

	fragColor = color;
}
)";

	struct NormalMapVertex final
	{
		glm::vec4 position;
		glm::vec3 tangent;
		glm::vec3 bitangent;
		glm::vec3 normal;
		glm::vec2 texCoord;
	};

	static constexpr auto NORMAL_MAP_VERTEX_SIZE = sizeof(NormalMapVertex);

	static const auto g_planeVertexDescs = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32A32_FLOAT,
			.offset   = offsetof(NormalMapVertex, position),
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 1,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = offsetof(NormalMapVertex, tangent),
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 2,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = offsetof(NormalMapVertex, bitangent),
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 3,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = offsetof(NormalMapVertex, normal),
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 4,
			.binding  = 0,
			.format   = gpu::Format::R32G32_FLOAT,
			.offset   = offsetof(NormalMapVertex, texCoord),
		},
	};

	[[nodiscard]] static std::pair<std::vector<NormalMapVertex>, std::vector<uint32_t>> CreatePlaneWithTangents(float size, int segments) noexcept
	{
		std::vector<NormalMapVertex> vertices;
		vertices.reserve((segments + 1) * (segments + 1));

		const float half = size * 0.5f;
		const float step = size / static_cast<float>(segments);
		const float uvRepeat = 2.0f;

		for (int row = 0; row <= segments; ++row)
		{
			float v = static_cast<float>(row) / static_cast<float>(segments) * uvRepeat;

			for (int col = 0; col <= segments; ++col)
			{
				float u = static_cast<float>(col) / static_cast<float>(segments) * uvRepeat;

				NormalMapVertex vert;
				vert.position  = glm::vec4(-half + col * step, -half + row * step, 0.0f, 1.0f);
				vert.tangent   = glm::vec3(1.0f, 0.0f, 0.0f);
				vert.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
				vert.normal    = glm::vec3(0.0f, 0.0f, 1.0f);
				vert.texCoord  = glm::vec2(u, v);

				vertices.push_back(vert);
			}
		}

		std::vector<uint32_t> indices;
		indices.reserve(segments * segments * 6);

		for (int row = 0; row < segments; ++row)
		{
			for (int col = 0; col < segments; ++col)
			{
				int bl = row * (segments + 1) + col;
				int br = row * (segments + 1) + (col + 1);
				int tl = (row + 1) * (segments + 1) + col;
				int tr = (row + 1) * (segments + 1) + (col + 1);

				// Two triangles forming a quad, CCW when viewed from +Z
				indices.push_back(bl);
				indices.push_back(br);
				indices.push_back(tr);
				indices.push_back(tr);
				indices.push_back(tl);
				indices.push_back(bl);
			}
		}

		return { std::move(vertices), std::move(indices) };
	}

	gpu::program::ShaderProgramPtr program;

	gpu::vao::VertexArrayPtr vao;
	gpu::buffer::BufferPtr vbo;
	gpu::buffer::BufferPtr ibo;
	uint32_t g_indexCount = 0;

	gpu::texture::TexturePtr colorTexture;
	gpu::texture::TexturePtr normalTexture;
	gpu::texture::SamplerPtr sampler;

	int g_projectionMatrixLocation = -1;
	int g_modelViewMatrixLocation  = -1;
	int g_normalMatrixLocation     = -1;
	int g_lightDirectionLocation   = -1;
	int g_textureLocation          = -1;
	int g_normalMapLocation        = -1;

	glm::mat4 g_viewMatrix(1.0f);

	gpu::DepthState depthState;
	gpu::RasterizationState defaultRasterState;
}
//=============================================================================
static bool GameInit()
{
	gpu::program::GraphicsProgramCreateInfo createInfo{
		.name = "NormalMap",
		.vertexShaderCode = vertexSource,
		.fragmentShaderCode = fragmentSource };
	program = gpu::program::CreateShaderProgram(createInfo);

	g_projectionMatrixLocation = gpu::program::GetUniformLocation(program, "u_projectionMatrix");
	g_modelViewMatrixLocation  = gpu::program::GetUniformLocation(program, "u_modelViewMatrix");
	g_normalMatrixLocation     = gpu::program::GetUniformLocation(program, "u_normalMatrix");
	g_lightDirectionLocation   = gpu::program::GetUniformLocation(program, "u_lightDirection");
	g_textureLocation   = gpu::program::GetUniformLocation(program, "u_texture");
	g_normalMapLocation = gpu::program::GetUniformLocation(program, "u_normalMap");

	auto [vertices, indices] = CreatePlaneWithTangents(1.5f, 8);
	g_indexCount = static_cast<uint32_t>(indices.size());

	vao = gpu::vao::CreateVertexArray(g_planeVertexDescs);
	vbo = gpu::buffer::CreateBuffer(vertices.data(), vertices.size() * sizeof(NormalMapVertex));
	ibo = gpu::buffer::CreateBuffer(indices.data(), indices.size() * sizeof(uint32_t));

	colorTexture  = gpu::texture::LoadTexture2D("data/textures/brickwall_albedo.png");
	normalTexture = gpu::texture::LoadTexture2D("data/textures/brickwall_normal.png");

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Linear;
	ss.magFilter = gpu::Filter::Linear;
	ss.addressModeU = gpu::AddressMode::Repeat;
	ss.addressModeV = gpu::AddressMode::Repeat;
	sampler = gpu::texture::CreateSampler(ss);

	// View: look from (0, 0, 5) at origin
	g_viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	gpu::program::BindShaderProgram(program);

	gpu::program::SetUniform(program, g_modelViewMatrixLocation, g_viewMatrix);

	glm::mat3 normalMatrix = glm::mat3(g_viewMatrix);
	gpu::program::SetUniform(program, g_normalMatrixLocation, normalMatrix);

	// Bind texture units
	gpu::program::SetUniform(program, g_textureLocation, 0);
	gpu::program::SetUniform(program, g_normalMapLocation, 1);

	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;

	return true;
}
//=============================================================================
static void GameClose()
{
	program.reset();
	vbo.reset();
	ibo.reset();
	vao.reset();
	colorTexture.reset();
	normalTexture.reset();
	sampler.reset();
}
//=============================================================================
static void GameUpdate()
{
	static float angle = 0.0f;
	float deltaTime = app::GetDeltaTime();

	angle += deltaTime;

	glm::vec3 lightDirection = glm::normalize(glm::vec3(2.0f * cosf(angle), 1.0f, 1.0f));

	// Transform light to camera space
	lightDirection = glm::vec3(g_viewMatrix * glm::vec4(lightDirection, 0.0f));

	gpu::program::SetUniform(program, g_lightDirectionLocation, lightDirection);

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

		gpu::cmd::BindSampledImage(0, colorTexture, sampler);
		gpu::cmd::BindSampledImage(1, normalTexture, sampler);

		gpu::cmd::BindVertexArray(vao);
		gpu::cmd::BindVertexBuffer(vao, 0, vbo, 0, NORMAL_MAP_VERTEX_SIZE);
		gpu::cmd::BindIndexBuffer(vao, ibo, gpu::IndexType::UnsignedInt);
		gpu::cmd::DrawIndexed(g_indexCount, 1, 0, 0, 0);
	}
	gpu::cmd::EndDraw();
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Normal Map");
	ImGui::Text("Example07: Normal Mapping");
	ImGui::Text("Light rotates around the object");
	ImGui::End();
}
//=============================================================================
void gpu004_normalMap()
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
