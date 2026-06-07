// Sprite/entity shader for rendering billboard sprites
constexpr auto g_spriteShaderVert = R"(
#version 460 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_worldPos;
uniform float u_size;

out vec2 v_texcoord;

void main()
{
	// Billboard: ignore rotation, use only view's up/right
	vec3 right = vec3(u_view[0][0], u_view[1][0], u_view[2][0]);
	vec3 up    = vec3(u_view[0][1], u_view[1][1], u_view[2][1]);
	vec3 pos = u_worldPos + (a_position.x * right + a_position.y * up) * u_size;
	v_texcoord = a_texcoord;
	gl_Position = u_projection * u_view * vec4(pos, 1.0);
}
)";

constexpr auto g_spriteShaderFrag = R"(
#version 460 core
in vec2 v_texcoord;
layout(location = 0) out vec4 o_color;

uniform bool u_hasTexture;
layout(binding = 0) uniform sampler2D u_texture;
uniform vec4 u_color;

void main()
{
	vec4 color = u_color;
	if (u_hasTexture) color *= texture(u_texture, v_texcoord);
	o_color = color;
}
)";