#include "stdafx.h"
//=============================================================================
namespace
{
	const char* vertexSource = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;

layout(location = 0) out vec3 v_eye;
layout(location = 1) out vec3 v_normal;

void main()
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
	v_eye = -vec3(gl_Position);
	v_normal = normalize(inverse(transpose(mat3(u_model))) * a_normal);
}
)";

	const char* fragmentSource = R"(
#version 460 core

struct LightProperties
{
	vec3 direction;
	vec4 ambientColor;
	vec4 diffuseColor;
	vec4 specularColor;
};

struct MaterialProperties
{
	vec4  ambientColor;
	vec4  diffuseColor;
	vec4  specularColor;
	float specularExponent;
};

layout(location = 0) in vec3 v_eye;
layout(location = 1) in vec3 v_normal;

uniform LightProperties    u_light;
uniform MaterialProperties u_material;

layout(location = 0) out vec4 fragColor;

void main()
{
	// Note: All calculations are in camera space.

	vec4 color = u_light.ambientColor * u_material.ambientColor;

	vec3 normal = normalize(v_normal);

	float nDotL = max(dot(u_light.direction, normal), 0.0);

	if (nDotL > 0.0)
	{
		vec3 eye = normalize(v_eye);

		// Incident vector is opposite light direction vector.
		vec3 reflection = reflect(-u_light.direction, normal);

		float eDotR = max(dot(eye, reflection), 0.0);

		color += u_light.diffuseColor * u_material.diffuseColor * nDotL;

		float specularIntensity = 0.0;

		if (eDotR > 0.0)
		{
			specularIntensity = pow(eDotR, u_material.specularExponent);
		}

		color += u_light.specularColor * u_material.specularColor * specularIntensity;
	}

	fragColor = color;
}
)";

	/**
	 * Properties of the light.
	 */
	struct LightProperties
	{
		glm::aligned_vec3 direction;
		glm::vec4 ambientColor;
		glm::vec4 diffuseColor;
		glm::vec4 specularColor;
	};

	/**
	 * Properties of the material, basically all the color factors without the emissive color component.
	 */
	struct MaterialProperties
	{
		glm::vec4 ambientColor;
		glm::vec4 diffuseColor;
		glm::vec4 specularColor;
		GLfloat specularExponent;
	};

	/**
	 * Locations for the light properties.
	 */
	struct LightLocations
	{
		GLint directionLocation;
		GLint ambientColorLocation;
		GLint diffuseColorLocation;
		GLint specularColorLocation;
	};

	/**
	* Locations for the material properties.
	*/
	struct MaterialLocations
	{
		GLint ambientColorLocation;
		GLint diffuseColorLocation;
		GLint specularColorLocation;
		GLint specularExponentLocation;
	};

	/**
	* The locations for the light properties.
	*/
	static LightLocations g_light;

	/**
	* The locations for the material properties.
	*/
	static MaterialLocations g_material;

	// This is a white light.
	LightProperties light = { {1.0f, 1.0f, 1.0f}, {0.3f, 0.3f, 0.3f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };

	// Blue color material with white specular color.
	MaterialProperties material = { {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, 20.0f };

	gpu::program::ShaderProgramPtr program;
	gr::Mesh mesh;

	gpu::texture::TexturePtr texture;
	gpu::texture::SamplerPtr sampler;

	gpu::uniform::Uniform<glm::mat4> proj;
	gpu::uniform::Uniform<glm::mat4> view;
	gpu::uniform::Uniform<glm::mat4> model;

	gpu::DepthState depthState;
	gpu::RasterizationState defaultRasterState;

	gr::Camera camera(glm::vec3(0.0f, 1.2f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	input::MouseLook mouseLook;
}
//=============================================================================
static bool GameInit()
{
	gpu::program::GraphicsProgramCreateInfo createInfo{
		.name = "Program",
		.vertexShaderCode = vertexSource,
		.fragmentShaderCode = fragmentSource };
	program = gpu::program::CreateShaderProgram(createInfo);

	gpu::uniform::InitUniform(proj, program, "u_projection");
	gpu::uniform::InitUniform(view, program, "u_view");
	gpu::uniform::InitUniform(model, program, "u_model");

	g_light.directionLocation = gpu::program::GetUniformLocation(program, "u_light.direction");
	g_light.ambientColorLocation = gpu::program::GetUniformLocation(program, "u_light.ambientColor");
	g_light.diffuseColorLocation = gpu::program::GetUniformLocation(program, "u_light.diffuseColor");
	g_light.specularColorLocation = gpu::program::GetUniformLocation(program, "u_light.specularColor");

	g_material.ambientColorLocation = gpu::program::GetUniformLocation(program, "u_material.ambientColor");
	g_material.diffuseColorLocation = gpu::program::GetUniformLocation(program, "u_material.diffuseColor");
	g_material.specularColorLocation = gpu::program::GetUniformLocation(program, "u_material.specularColor");
	g_material.specularExponentLocation = gpu::program::GetUniformLocation(program, "u_material.specularExponent");

	mesh = gr::Mesh::CreateSphere(32, 32);
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
static void GameClose()
{
	mouseLook.Reset();
	program.reset();
	mesh.Close();
}
//=============================================================================
static void GameUpdate()
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

	// Set up light ...
	gpu::program::SetUniform(program, g_light.directionLocation, light.direction);
	gpu::program::SetUniform(program, g_light.ambientColorLocation, light.ambientColor);
	gpu::program::SetUniform(program, g_light.diffuseColorLocation, light.diffuseColor);
	gpu::program::SetUniform(program, g_light.specularColorLocation, light.specularColor);

	// ... and material values.
	gpu::program::SetUniform(program, g_material.ambientColorLocation, material.ambientColor);
	gpu::program::SetUniform(program, g_material.diffuseColorLocation, material.diffuseColor);
	gpu::program::SetUniform(program, g_material.specularColorLocation, material.specularColor);
	gpu::program::SetUniform(program, g_material.specularExponentLocation, material.specularExponent);
}
//=============================================================================
static void GameFixedUpdate()
{}
//=============================================================================
static void GameRender()
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
		// Cube
		gpu::cmd::SetState(depthState);
		gpu::cmd::SetState(defaultRasterState);
		gpu::cmd::BindShaderProgram(program);
		gpu::cmd::BindSampledImage(0, texture, sampler);
		mesh.Bind();
		mesh.Draw();
	}
	gpu::cmd::EndDraw();
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Hello, world!");
	ImGui::Text("This is some useful text.");
	ImGui::End();
}
//=============================================================================
void gpu003_spherePhong()
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