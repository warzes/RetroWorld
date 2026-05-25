#include "stdafx.h"
//=============================================================================
namespace
{
	const char* vertexSource = R"(
#version 460 core

layout(location = 0) in vec3 vpoint;

layout(location = 0) out float red;
layout(location = 1) out float green;
layout(location = 2) out float blue;

void main()
{
    gl_Position = vec4(vpoint, 1.0);
    red = green = blue = 0.0;

    if(gl_VertexID == 0){
        red = 1.0;
    }
    else if(gl_VertexID == 1){
        green = 1.0;
    }
    else{
        blue = 1.0;
    }
}
)";

	const char* fragmentSource = R"(
#version 460 core

layout(location = 0) in float red;
layout(location = 1) in float green;
layout(location = 2) in float blue;

layout(location = 0) out vec4 o_color;

void main()
{
    o_color = vec4(red, green, blue, 1.0);
}
)";

	const GLfloat vpoint[] = {
	   -1.0f, -1.0f, 0.0f,
	   1.0f, -1.0f, 0.0f,
	   0.0f,  1.0f, 0.0f, };

	gpu::program::ShaderProgramPtr program;

	GLuint vao;
	GLuint vbo;
}
//=============================================================================
bool GameInit()
{
	program = gpu::program::CreateShaderProgram(vertexSource, fragmentSource);

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vpoint), vpoint, GL_STATIC_DRAW);

	//assign triangle point as vpoint to the shader
	gpu::program::BindShaderProgram(program);
	GLuint vpoint_id = gpu::program::GetAttributeLocation(program, "vpoint");
	glEnableVertexAttribArray(vpoint_id);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(vpoint_id, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	return true;
}
//=============================================================================
void GameClose()
{
}
//=============================================================================
void GameUpdate()
{
}
//=============================================================================
void GameFixedUpdate()
{
}
//=============================================================================
void GameRender()
{
	gpu::program::BindShaderProgram(program);
	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLES, 0, 3);
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