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

	gpu::DepthState depthState;

	gr::Camera camera(glm::vec3(0.0f, 1.2f, -3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
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

	texture = gpu::texture::LoadTexture2D("data/textures/uv.png");

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Nearest;
	ss.magFilter = gpu::Filter::Nearest;
	ss.addressModeU = gpu::AddressMode::Repeat;
	ss.addressModeV = gpu::AddressMode::Repeat;
	sampler = gpu::texture::CreateSampler(ss);

	depthState.depthTestEnable = true;

	return true;
}
//=============================================================================
void GameClose()
{
	g_mouseLook.Reset();
	program.reset();
	vao.reset();
	vbo.reset();
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

	//if (input::IsMouseDown(MouseType::MOUSE_BUTTON_RIGHT))
	//{
	//	if (!mouseCapture)
	//	{
	//		prevMouse = input::GetMousePosition();
	//		mouseCapture = true;
	//		input::SetCursorVisible(false);
	//	}
	//}
	//else
	//{
	//	if (mouseCapture)
	//	{
	//		input::SetCursorVisible(true);
	//		mouseCapture = false;
	//	}
	//}

	//if (mouseCapture)
	//{
	//	math::point2 delta = input::GetMousePosition() - prevMouse;
	//	camera.Rotate(-delta.y * 0.1f, delta.x * 0.1f, 0.0f);

	//	input::SetMousePosition(window::GetWidth() / 2, window::GetHeight() / 2);
	//	prevMouse = { window::GetWidth() / 2, window::GetHeight() / 2 };
	//}

	proj = glm::perspective(glm::radians(65.f), window::GetAspectRatio(), 0.1f, 1000.f);
	view = camera.GetViewMatrix();
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
	gpu::cmd::SetState(depthState);
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