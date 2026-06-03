#include "stdafx.h"
//=============================================================================
namespace
{
	const char* glassVertexSource = R"(
#version 460 core

const float Air   = 1.0;
const float Glass = 1.51714;
const float Eta   = Air / Glass;
const float R0    = ((Air - Glass) * (Air - Glass)) / ((Air + Glass) * (Air + Glass));

uniform mat4 u_viewProjectionMatrix;
uniform mat4 u_modelMatrix;
uniform mat3 u_normalMatrix;
uniform vec4 u_camera;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;

layout(location = 0) out vec3  v_reflection;
layout(location = 1) out vec3  v_refraction;
layout(location = 2) out float v_fresnel;

void main()
{
	vec4 vertex = u_modelMatrix * a_vertex;

	vec3 incident = normalize(vec3(vertex - u_camera));

	vec3 normal = u_normalMatrix * a_normal;

	v_refraction = refract(incident, normal, Eta);
	v_reflection = reflect(incident, normal);

	v_fresnel = R0 + (1.0 - R0) * pow((1.0 - dot(-incident, normal)), 5.0);

	gl_Position = u_viewProjectionMatrix * vertex;
}
)";

	const char* glassFragmentSource = R"(
#version 460 core

layout(binding = 0) uniform samplerCube u_cubemap;

layout(location = 0) in vec3  v_refraction;
layout(location = 1) in vec3  v_reflection;
layout(location = 2) in float v_fresnel;

layout(location = 0) out vec4 fragColor;

void main()
{
	vec4 refractionColor = texture(u_cubemap, normalize(v_refraction));
	vec4 reflectionColor = texture(u_cubemap, normalize(v_reflection));

	fragColor = mix(refractionColor, reflectionColor, v_fresnel);
}
)";

	const char* bgVertexSource = R"(
#version 460 core

uniform mat4 u_viewProjectionMatrix;
uniform mat4 u_modelMatrix;

layout(location = 0) in vec4 a_vertex;

layout(location = 0) out vec3 v_ray;

void main()
{
	v_ray = normalize(a_vertex.xyz);

	gl_Position = u_viewProjectionMatrix * a_vertex;
}
)";

	const char* bgFragmentSource = R"(
#version 460 core

layout(binding = 0) uniform samplerCube u_cubemap;

layout(location = 0) in vec3 v_ray;

layout(location = 0) out vec4 fragColor;

void main()
{
	fragColor = texture(u_cubemap, v_ray);
}
)";

	//=======================================================================
	// Vertex types
	//=======================================================================
	struct TorusVertex final
	{
		glm::vec4 position;
		glm::vec3 normal;
	};

	struct BgVertex final
	{
		glm::vec4 position;
	};

	//=======================================================================
	// VAO binding descriptions
	//=======================================================================
	static const auto g_torusVertexDescs = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32A32_FLOAT,
			.offset   = offsetof(TorusVertex, position),
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 1,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = offsetof(TorusVertex, normal),
		},
	};

	static const auto g_bgVertexDescs = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32A32_FLOAT,
			.offset   = 0,
		},
	};

	//=======================================================================
	// Shape generation
	//=======================================================================
	[[nodiscard]] static std::pair<std::vector<TorusVertex>, std::vector<uint32_t>> CreateTorus(
		float innerRadius, float outerRadius, int sides, int rings) noexcept
	{
		std::vector<TorusVertex> vertices;
		vertices.reserve((rings + 1) * (sides + 1));

		for (int ring = 0; ring <= rings; ++ring)
		{
			float u = 2.0f * glm::pi<float>() * static_cast<float>(ring) / static_cast<float>(rings);

			for (int side = 0; side <= sides; ++side)
			{
				float v = 2.0f * glm::pi<float>() * static_cast<float>(side) / static_cast<float>(sides);

				float cu = cosf(u), su = sinf(u);
				float cv = cosf(v), sv = sinf(v);

				TorusVertex vert;
				vert.position = glm::vec4(
					(outerRadius + innerRadius * cv) * cu,
					innerRadius * sv,
					(outerRadius + innerRadius * cv) * su,
					1.0f);
				vert.normal = glm::vec3(cv * cu, sv, cv * su);
				vertices.push_back(vert);
			}
		}

		std::vector<uint32_t> indices;
		indices.reserve(rings * sides * 6);

		for (int ring = 0; ring < rings; ++ring)
		{
			for (int side = 0; side < sides; ++side)
			{
				int i0 = ring * (sides + 1) + side;
				int i1 = ring * (sides + 1) + (side + 1);
				int i2 = (ring + 1) * (sides + 1) + side;
				int i3 = (ring + 1) * (sides + 1) + (side + 1);

				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);
				indices.push_back(i2);
				indices.push_back(i1);
				indices.push_back(i3);
			}
		}

		return { std::move(vertices), std::move(indices) };
	}

	[[nodiscard]] static std::pair<std::vector<BgVertex>, std::vector<uint32_t>> CreateBgSphere(
		float radius, int segments) noexcept
	{
		std::vector<BgVertex> vertices;
		vertices.reserve((segments + 1) * (segments + 1));

		for (int r = 0; r <= segments; ++r)
		{
			float phi = glm::pi<float>() * static_cast<float>(r) / static_cast<float>(segments);

			for (int s = 0; s <= segments; ++s)
			{
				float theta = 2.0f * glm::pi<float>() * static_cast<float>(s) / static_cast<float>(segments);

				float sp = sinf(phi), cp = cosf(phi);
				float st = sinf(theta), ct = cosf(theta);

				BgVertex vert;
				vert.position = glm::vec4(
					radius * sp * ct,
					radius * cp,
					radius * sp * st,
					1.0f);
				vertices.push_back(vert);
			}
		}

		std::vector<uint32_t> indices;
		indices.reserve(segments * segments * 6);

		for (int r = 0; r < segments; ++r)
		{
			for (int s = 0; s < segments; ++s)
			{
				int i0 = r * (segments + 1) + s;
				int i1 = r * (segments + 1) + (s + 1);
				int i2 = (r + 1) * (segments + 1) + s;
				int i3 = (r + 1) * (segments + 1) + (s + 1);

				indices.push_back(i0);
				indices.push_back(i1);
				indices.push_back(i2);
				indices.push_back(i2);
				indices.push_back(i1);
				indices.push_back(i3);
			}
		}

		return { std::move(vertices), std::move(indices) };
	}

	//=======================================================================
	// Resources
	//=======================================================================
	gpu::program::ShaderProgramPtr glassProgram;
	gpu::program::ShaderProgramPtr bgProgram;

	gpu::vao::VertexArrayPtr torusVao;
	gpu::buffer::BufferPtr torusVbo;
	gpu::buffer::BufferPtr torusIbo;
	uint32_t g_torusIndexCount = 0;

	gpu::vao::VertexArrayPtr bgVao;
	gpu::buffer::BufferPtr bgVbo;
	gpu::buffer::BufferPtr bgIbo;
	uint32_t g_bgIndexCount = 0;

	gpu::texture::TexturePtr cubemapTexture;
	gpu::texture::SamplerPtr sampler;

	int g_viewProjLocation       = -1;
	int g_modelMatrixLocation    = -1;
	int g_normalMatrixLocation   = -1;
	int g_cameraLocation         = -1;

	int g_bgViewProjLocation     = -1;
	int g_bgModelMatrixLocation  = -1;

	glm::mat4 g_projectionMatrix(1.0f);

	float g_circleRadius = 5.0f;

	gpu::DepthState depthState;
	gpu::RasterizationState defaultRasterState;
	gpu::RasterizationState bgRasterState;
}
//=============================================================================
static bool GameInit()
{
	// ---- Glass program ----
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "GlassProgram",
			.vertexShaderCode = glassVertexSource,
			.fragmentShaderCode = glassFragmentSource };
		glassProgram = gpu::program::CreateShaderProgram(info);

		g_viewProjLocation     = gpu::program::GetUniformLocation(glassProgram, "u_viewProjectionMatrix");
		g_modelMatrixLocation  = gpu::program::GetUniformLocation(glassProgram, "u_modelMatrix");
		g_normalMatrixLocation = gpu::program::GetUniformLocation(glassProgram, "u_normalMatrix");
		g_cameraLocation       = gpu::program::GetUniformLocation(glassProgram, "u_camera");
	}

	// ---- Background program ----
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "BgProgram",
			.vertexShaderCode = bgVertexSource,
			.fragmentShaderCode = bgFragmentSource };
		bgProgram = gpu::program::CreateShaderProgram(info);

		g_bgViewProjLocation    = gpu::program::GetUniformLocation(bgProgram, "u_viewProjectionMatrix");
		g_bgModelMatrixLocation = gpu::program::GetUniformLocation(bgProgram, "u_modelMatrix");
	}

	// ---- Torus geometry ----
	{
		auto [vertices, indices] = CreateTorus(0.25f, 1.0f, 32, 32);
		g_torusIndexCount = static_cast<uint32_t>(indices.size());

		torusVao = gpu::vao::CreateVertexArray(g_torusVertexDescs);
		torusVbo = gpu::buffer::CreateBuffer(vertices.data(), vertices.size() * sizeof(TorusVertex));
		torusIbo = gpu::buffer::CreateBuffer(indices.data(), indices.size() * sizeof(uint32_t));
	}

	// ---- Background sphere geometry ----
	{
		auto [vertices, indices] = CreateBgSphere(g_circleRadius, 32);
		g_bgIndexCount = static_cast<uint32_t>(indices.size());

		bgVao = gpu::vao::CreateVertexArray(g_bgVertexDescs);
		bgVbo = gpu::buffer::CreateBuffer(vertices.data(), vertices.size() * sizeof(BgVertex));
		bgIbo = gpu::buffer::CreateBuffer(indices.data(), indices.size() * sizeof(uint32_t));
	}

	// ---- Cubemap texture ----
	{
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
	}

	// ---- State ----
	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;

	bgRasterState.cullMode = gpu::CullMode::Front;

	return true;
}
//=============================================================================
static void GameClose()
{
	glassProgram.reset();
	bgProgram.reset();
	torusVao.reset();
	torusVbo.reset();
	torusIbo.reset();
	bgVao.reset();
	bgVbo.reset();
	bgIbo.reset();
	cubemapTexture.reset();
	sampler.reset();
}
//=============================================================================
static void GameUpdate()
{
	static float angle = 0.0f;
	float deltaTime = app::GetDeltaTime();

	angle += 30.0f * deltaTime;

	float angleRad = glm::radians(angle);

	glm::vec3 cameraPos(
		g_circleRadius * -sinf(angleRad),
		0.0f,
		g_circleRadius * cosf(angleRad));

	glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	g_projectionMatrix = glm::perspective(glm::radians(40.0f), window::GetAspectRatio(), 1.0f, 100.0f);
	glm::mat4 viewProj = g_projectionMatrix * view;

	// Model: translate(0, -0.5, 0) * rotateX(45)
	glm::mat4 model(1.0f);
	model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
	model = glm::rotate(model, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));

	glm::mat3 normalMatrix = glm::mat3(model);

	glm::vec4 cameraUniform(cameraPos, 1.0f);

	// Glass program uniforms
	gpu::program::SetUniform(glassProgram, g_viewProjLocation, viewProj);
	gpu::program::SetUniform(glassProgram, g_modelMatrixLocation, model);
	gpu::program::SetUniform(glassProgram, g_normalMatrixLocation, normalMatrix);
	gpu::program::SetUniform(glassProgram, g_cameraLocation, cameraUniform);

	// Background program uniforms
	gpu::program::SetUniform(bgProgram, g_bgViewProjLocation, viewProj);
	gpu::program::SetUniform(bgProgram, g_bgModelMatrixLocation, model);
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
		// ---- Background sphere (inside view) ----
		gpu::cmd::SetState(depthState);
		gpu::cmd::SetState(bgRasterState);
		gpu::cmd::BindShaderProgram(bgProgram);

		gpu::cmd::BindSampledImage(0, cubemapTexture, sampler);
		gpu::cmd::BindVertexArray(bgVao);
		gpu::cmd::BindVertexBuffer(bgVao, 0, bgVbo, 0, sizeof(BgVertex));
		gpu::cmd::BindIndexBuffer(bgVao, bgIbo, gpu::IndexType::UnsignedInt);
		gpu::cmd::DrawIndexed(g_bgIndexCount, 1, 0, 0, 0);

		// ---- Glass torus ----
		gpu::cmd::SetState(depthState);
		gpu::cmd::SetState(defaultRasterState);
		gpu::cmd::BindShaderProgram(glassProgram);

		gpu::cmd::BindSampledImage(0, cubemapTexture, sampler);
		gpu::cmd::BindVertexArray(torusVao);
		gpu::cmd::BindVertexBuffer(torusVao, 0, torusVbo, 0, sizeof(TorusVertex));
		gpu::cmd::BindIndexBuffer(torusVao, torusIbo, gpu::IndexType::UnsignedInt);
		gpu::cmd::DrawIndexed(g_torusIndexCount, 1, 0, 0, 0);
	}
	gpu::cmd::EndDraw();
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Reflection && Refraction");
	ImGui::Text("Example11: Glass Torus with Fresnel");
	ImGui::Text("Camera orbits around the scene");
	ImGui::End();
}
//=============================================================================
void gpu006_reflectionAndRefraction()
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
