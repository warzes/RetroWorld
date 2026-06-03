#include "stdafx.h"
#include <glm/gtc/matrix_transform.hpp>
//=============================================================================
namespace
{
	//=======================================================================
	// Constants matching Example14
	//=======================================================================
	static constexpr float TURN_DURATION                 = 20.0f;
	static constexpr float TURN_RADIUS                    = 6000.0f;
	static constexpr float HORIZONTAL_PIXEL_SPACING       = 60.0f;
	static constexpr float VERTICAL_PIXEL_RANGE           = 10004.0f;
	static constexpr float METERS_TO_VIRTUAL_WORLD_SCALE  = 5.0f;
	static constexpr uint32_t MINIMUM_DETAIL_LEVEL        = 4;
	static constexpr uint32_t DETAIL_LEVEL_FIRST_PASS     = 2;
	static constexpr float   FOV_RADIUS                   = 10000.0f;
	static constexpr int     QUADRANT_STEP                = 2;

	static constexpr float HORIZ_SCALE = HORIZONTAL_PIXEL_SPACING * METERS_TO_VIRTUAL_WORLD_SCALE;
	static constexpr float VERT_SCALE   = VERTICAL_PIXEL_RANGE * METERS_TO_VIRTUAL_WORLD_SCALE;

	//=======================================================================
	// Shader sources
	//=======================================================================
	const char* passOneVertexSrc = R"(
#version 410 core

layout(location = 0) in vec2 a_vertex;

void main(void)
{
	gl_Position = vec4(a_vertex.x, 0.0, a_vertex.y, 1.0);
}
)";

	const char* passOneGeometrySrc = R"(
#version 410 core

layout(points, invocations = 1) in;
layout(points, max_vertices = 64) out;

uniform float u_halfDetailStep;
uniform uint  u_detailLevel;
uniform float u_fovRadius;

uniform vec4 u_positionTextureSpace;
uniform vec3 u_leftNormalTextureSpace;
uniform vec3 u_rightNormalTextureSpace;
uniform vec3 u_backNormalTextureSpace;

out vec2 v_vertex;

bool isOutside(vec4 point, vec4 viewPoint, float step)
{
	float bias = 0.1;

	vec3 viewVector = vec3(point - viewPoint);

	float boundingRadius = sqrt(step * step + step * step);

	if (length(viewVector) - boundingRadius > u_fovRadius)
	{
		return true;
	}
	if (dot(viewVector, u_backNormalTextureSpace) > boundingRadius + bias)
	{
		return true;
	}
	if (dot(viewVector, u_leftNormalTextureSpace) > boundingRadius + bias)
	{
		return true;
	}
	if (dot(viewVector, u_rightNormalTextureSpace) > boundingRadius + bias)
	{
		return true;
	}

	return false;
}

void main(void)
{
	if (isOutside(gl_in[0].gl_Position, u_positionTextureSpace, u_halfDetailStep))
	{
		return;
	}

	uint steps = uint(pow(2.0, float(u_detailLevel)));

	float finalDetailStep = u_halfDetailStep * 2.0 / float(steps);

	float halfFinalDetailStep = finalDetailStep / 2.0;

	vec4 centerPoint;

	float xFloat;
	float zFloat;

	for (uint z = 0u; z < steps; z++)
	{
		zFloat = float(z);

		for (uint x = 0u; x < steps; x++)
		{
			xFloat = float(x);

			centerPoint = vec4(gl_in[0].gl_Position.x + xFloat * finalDetailStep - u_halfDetailStep + halfFinalDetailStep, 0.0, gl_in[0].gl_Position.z + zFloat * finalDetailStep - u_halfDetailStep + halfFinalDetailStep, 1.0);

			if (isOutside(centerPoint, u_positionTextureSpace, halfFinalDetailStep))
			{
				continue;
			}

			v_vertex = vec2(gl_in[0].gl_Position.x + xFloat * finalDetailStep - u_halfDetailStep, gl_in[0].gl_Position.z + zFloat * finalDetailStep - u_halfDetailStep);
			EmitVertex();
			EndPrimitive();

			v_vertex = vec2(gl_in[0].gl_Position.x + (xFloat + 1.0) * finalDetailStep - u_halfDetailStep, gl_in[0].gl_Position.z + zFloat * finalDetailStep - u_halfDetailStep);
			EmitVertex();
			EndPrimitive();

			v_vertex = vec2(gl_in[0].gl_Position.x + (xFloat + 1.0) * finalDetailStep - u_halfDetailStep, gl_in[0].gl_Position.z + (zFloat + 1.0) * finalDetailStep - u_halfDetailStep);
			EmitVertex();
			EndPrimitive();

			v_vertex = vec2(gl_in[0].gl_Position.x + xFloat * finalDetailStep - u_halfDetailStep, gl_in[0].gl_Position.z + (zFloat + 1.0) * finalDetailStep - u_halfDetailStep);
			EmitVertex();
			EndPrimitive();
		}
	}
}
)";

	const char* passOneFragmentSrc = R"(
#version 410 core

layout(location = 0, index = 0) out vec4 fragColor;

void main(void)
{
	fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
)";

	//=======================================================================

	const char* passTwoVertexSrc = R"(
#version 410 core

layout(location = 0) in vec2 a_vertex;

void main(void)
{
	gl_Position = vec4(a_vertex.x, 0.0, a_vertex.y, 1.0);
}
)";

	const char* passTwoControlSrc = R"(
#version 410 core

layout(vertices = 4) out;

uniform uint u_maxTessellationLevel;
uniform int  u_quadrantStep;
uniform vec4 u_positionTextureSpace;

void main(void)
{
	float tessellationStep = pow(2.0, float(u_maxTessellationLevel));

	int relativeQuadrantS = int((gl_in[0].gl_Position.x - u_positionTextureSpace.x) / tessellationStep);
	int relativeQuadrantT = int((gl_in[0].gl_Position.z - u_positionTextureSpace.z) / tessellationStep);

	int absRelativeQuadrantS = abs(relativeQuadrantS);
	int absRelativeQuadrantT = abs(relativeQuadrantT);

	int chebyshevDistance = max(absRelativeQuadrantS, absRelativeQuadrantT);

	bool leftBorder  = ((absRelativeQuadrantS + 1) % u_quadrantStep == 0) && (absRelativeQuadrantS == chebyshevDistance) && (relativeQuadrantS <= 0);
	bool rightBorder = ((absRelativeQuadrantS + 1) % u_quadrantStep == 0) && (absRelativeQuadrantS == chebyshevDistance) && (relativeQuadrantS >= 0);

	bool bottomBorder = ((absRelativeQuadrantT + 1) % u_quadrantStep == 0) && (absRelativeQuadrantT == chebyshevDistance) && (relativeQuadrantT <= 0);
	bool topBorder    = ((absRelativeQuadrantT + 1) % u_quadrantStep == 0) && (absRelativeQuadrantT == chebyshevDistance) && (relativeQuadrantT >= 0);

	uint tessellationLevel = uint(chebyshevDistance / u_quadrantStep);

	uint decrease = min(u_maxTessellationLevel + 1u, tessellationLevel);

	uint decreaseLeft   = decrease;
	uint decreaseRight  = decrease;
	uint decreaseTop    = decrease;
	uint decreaseBottom = decrease;
	uint decreaseInner  = decrease;

	decrease = min(u_maxTessellationLevel + 1u, decrease + 1u);

	if (leftBorder)
	{
		decreaseLeft = decrease;
	}
	if (rightBorder)
	{
		decreaseRight = decrease;
	}
	if (bottomBorder)
	{
		decreaseBottom = decrease;
	}
	if (topBorder)
	{
		decreaseTop = decrease;
	}

	gl_TessLevelOuter[0] = pow(2.0, float(u_maxTessellationLevel + 1u - decreaseLeft));
	gl_TessLevelOuter[1] = pow(2.0, float(u_maxTessellationLevel + 1u - decreaseBottom));
	gl_TessLevelOuter[2] = pow(2.0, float(u_maxTessellationLevel + 1u - decreaseRight));
	gl_TessLevelOuter[3] = pow(2.0, float(u_maxTessellationLevel + 1u - decreaseTop));

	gl_TessLevelInner[0] = pow(2.0, float(u_maxTessellationLevel + 1u - decreaseInner));
	gl_TessLevelInner[1] = pow(2.0, float(u_maxTessellationLevel + 1u - decreaseInner));

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}
)";

	const char* passTwoEvalSrc = R"(
#version 410 core

layout(quads, equal_spacing, ccw) in;

vec4 interpolate(in vec4 v0, in vec4 v1, in vec4 v2, in vec4 v3)
{
	vec4 a = mix(v0, v1, gl_TessCoord.x);
	vec4 b = mix(v3, v2, gl_TessCoord.x);
	return mix(a, b, gl_TessCoord.y);
}

void main()
{
	gl_Position = interpolate(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_in[2].gl_Position, gl_in[3].gl_Position);
}
)";

	const char* passTwoGeometrySrc = R"(
#version 410 core

layout(triangles, invocations = 1) in;
layout(triangle_strip, max_vertices = 3) out;

uniform sampler2DRect u_heightMapTexture;
uniform sampler2DRect u_normalMapTexture;

uniform mat4  u_tmvpMatrix;
uniform vec3  u_lightDirection;

out vec2  v_texCoord;
out float v_intensity;

void main(void)
{
	ivec2 heightMapTextureSize = textureSize(u_heightMapTexture);

	vec4 heightMapPosition;
	vec3 normal;

	for (int i = 0; i < gl_in.length(); ++i)
	{
		heightMapPosition = gl_in[i].gl_Position;

		heightMapPosition.y = texture(u_heightMapTexture, heightMapPosition.xz).r;

		v_texCoord = vec2((heightMapPosition.x - 0.5) / heightMapTextureSize.s, (heightMapPosition.z - 0.5) / heightMapTextureSize.t);

		normal = texture(u_normalMapTexture, heightMapPosition.xz).xyz * 2.0 - 1.0;

		v_intensity = max(dot(normalize(normal), u_lightDirection), 0.0);

		gl_Position = u_tmvpMatrix * heightMapPosition;

		EmitVertex();
	}

	EndPrimitive();
}
)";

	const char* passTwoFragmentSrc = R"(
#version 410 core

layout(location = 0, index = 0) out vec4 fragColor;

uniform sampler2D u_colorMapTexture;

in vec2  v_texCoord;
in float v_intensity;

void main(void)
{
	fragColor = texture(u_colorMapTexture, v_texCoord) * v_intensity;
}
)";

	//=======================================================================
	// Texture helpers (raw GL for rectangle textures)
	//=======================================================================
	[[nodiscard]] static GLuint CreateRectTextureFromFile(const char* path) noexcept
	{
		int w, h, comp;
		stbi_set_flip_vertically_on_load(false);
		auto* data = stbi_load(path, &w, &h, &comp, 4);
		assert(data && "Failed to load rectangle texture");

		GLuint tex = 0;
		glCreateTextures(GL_TEXTURE_RECTANGLE, 1, &tex);
		glTextureStorage2D(tex, 1, GL_RGBA8, w, h);
		glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		stbi_image_free(data);
		return tex;
	}

	[[nodiscard]] static GLuint Create2DTextureFromFile(const char* path) noexcept
	{
		int w, h, comp;
		stbi_set_flip_vertically_on_load(false);
		auto* data = stbi_load(path, &w, &h, &comp, 4);
		assert(data && "Failed to load 2D texture");

		GLuint tex = 0;
		glCreateTextures(GL_TEXTURE_2D, 1, &tex);
		glTextureStorage2D(tex, 1, GL_RGBA8, w, h);
		glTextureSubImage2D(tex, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);

		glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(tex, GL_TEXTURE_WRAP_T, GL_REPEAT);

		stbi_image_free(data);
		return tex;
	}

	//=======================================================================
	// Compile a single shader stage
	//=======================================================================
	[[nodiscard]] static GLuint CompileShaderGL(GLenum type, const char* source) noexcept
	{
		GLuint shader = glCreateShader(type);
		glShaderSource(shader, 1, &source, nullptr);
		glCompileShader(shader);

		GLint status = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (!status)
		{
			char log[1024];
			GLsizei len = 0;
			glGetShaderInfoLog(shader, sizeof(log), &len, log);
			assert(false && "Shader compilation failed");
		}
		return shader;
	}

	//=======================================================================
	// Resources
	//=======================================================================

	// Pass One (raw GL program with transform feedback)
	static GLuint g_passOneProgram = 0;
	static GLint  g_halfDetailStepPassOneLocation       = -1;
	static GLint  g_detailLevelPassOneLocation           = -1;
	static GLint  g_fovRadiusPassOneLocation             = -1;
	static GLint  g_positionTextureSpacePassOneLocation  = -1;
	static GLint  g_leftNormalTextureSpacePassOneLocation  = -1;
	static GLint  g_rightNormalTextureSpacePassOneLocation = -1;
	static GLint  g_backNormalTextureSpacePassOneLocation  = -1;

	// Pass Two (engine program)
	gpu::program::ShaderProgramPtr g_passTwoProgram;
	static GLint g_tmvpPassTwoLocation                     = -1;
	static GLint g_maxTessellationLevelPassTwoLocation     = -1;
	static GLint g_quadrantStepPassTwoLocation             = -1;
	static GLint g_positionTextureSpacePassTwoLocation     = -1;
	static GLint g_heightMapTexturePassTwoLocation         = -1;
	static GLint g_colorMapTexturePassTwoLocation          = -1;
	static GLint g_normalMapTexturePassTwoLocation         = -1;
	static GLint g_lightDirectionPassTwoLocation           = -1;

	// Buffers
	static gpu::buffer::BufferPtr g_passOneVBO;
	static gpu::buffer::BufferPtr g_passTwoVBO;
	static uint32_t g_sNumPoints = 0;
	static uint32_t g_tNumPoints = 0;

	// VAOs
	static gpu::vao::VertexArrayPtr g_vaoPassOne;
	static gpu::vao::VertexArrayPtr g_vaoPassTwo;

	// Textures (raw GL handles)
	static GLuint g_heightMapTexture = 0;
	static GLuint g_normalMapTexture = 0;
	static GLuint g_colorMapTexture  = 0;

	// Transform feedback query
	static GLuint g_transformFeedbackQuery = 0;

	// Matrices
	static glm::mat4 g_projectionMatrix(1.0f);
	static glm::mat4 g_textureToWorldMatrix(1.0f);
	static glm::mat4 g_worldToTextureMatrix(1.0f);
	static glm::mat3 g_worldToTextureNormalMatrix(1.0f);

	// Camera state
	static float g_angle = 0.0f;
	static bool  g_animationOn = true;

	// Texture dimensions (rectangle)
	static float g_sMapExtend = 0.0f;
	static float g_tMapExtend = 0.0f;

	// Overall max detail level
	static uint32_t g_overallMaxDetailLevel = 0;

	// Default light direction
	static glm::vec3 g_lightDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));

	// State
	gpu::DepthState depthState;
	gpu::RasterizationState defaultRasterState;

	// VAO descriptions
	static const auto g_vec2VertexDescs = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32_FLOAT,
			.offset   = 0,
		},
	};
} // anonymous namespace
//=============================================================================
static bool GameInit()
{
	// ---- Load height map to determine dimensions ----
	{
		int w, h, comp;
		stbi_set_flip_vertically_on_load(false);
		auto* data = stbi_load("data/textures/grand_canyon_height.tga", &w, &h, &comp, 4);
		assert(data && "Failed to load heightmap");

		g_sMapExtend = static_cast<float>(w);
		g_tMapExtend = static_cast<float>(h);

		// Create rectangle texture
		glCreateTextures(GL_TEXTURE_RECTANGLE, 1, &g_heightMapTexture);
		glTextureStorage2D(g_heightMapTexture, 1, GL_RGBA8, w, h);
		glTextureSubImage2D(g_heightMapTexture, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTextureParameteri(g_heightMapTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(g_heightMapTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(g_heightMapTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(g_heightMapTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		stbi_image_free(data);
	}

	// ---- Load normal map ----
	{
		g_normalMapTexture = CreateRectTextureFromFile("data/textures/grand_canyon_normal.tga");
	}

	// ---- Load color map ----
	{
		g_colorMapTexture = Create2DTextureFromFile("data/textures/grand_canyon_color.tga");
	}

	// ---- Compute detail levels ----
	{
		uint32_t sMaxDetailLevel = static_cast<uint32_t>(floorf(logf(g_sMapExtend) / logf(2.0f)));
		uint32_t tMaxDetailLevel = static_cast<uint32_t>(floorf(logf(g_tMapExtend) / logf(2.0f)));
		g_overallMaxDetailLevel = std::min(sMaxDetailLevel, tMaxDetailLevel);

		assert(MINIMUM_DETAIL_LEVEL <= g_overallMaxDetailLevel && "Detail level too high");
		assert(MINIMUM_DETAIL_LEVEL + DETAIL_LEVEL_FIRST_PASS <= g_overallMaxDetailLevel && "First pass detail level too high");

		float detailStep = powf(2.0f, static_cast<float>(g_overallMaxDetailLevel - MINIMUM_DETAIL_LEVEL));

		g_sNumPoints = static_cast<uint32_t>(ceilf(g_sMapExtend / detailStep)) - 1;
		g_tNumPoints = static_cast<uint32_t>(ceilf(g_tMapExtend / detailStep)) - 1;

		// ---- Generate initial vertex grid (vec2 positions in texture space) ----
		std::vector<float> map;
		map.reserve(static_cast<size_t>(g_sNumPoints) * g_tNumPoints * 2);

		for (uint32_t t = 0; t < g_tNumPoints; t++)
		{
			for (uint32_t s = 0; s < g_sNumPoints; s++)
			{
				map.push_back(0.5f + detailStep / 2.0f + static_cast<float>(s) * detailStep);
				map.push_back(0.5f + detailStep / 2.0f + static_cast<float>(t) * detailStep);
			}
		}

		// ---- Create buffers ----
		size_t vertexCount = static_cast<size_t>(g_sNumPoints) * g_tNumPoints;
		size_t vertexDataSize = vertexCount * 2 * sizeof(float);

		g_passOneVBO = gpu::buffer::CreateBuffer(map.data(), vertexDataSize);

		// PassTwo VBO: large enough for subdivided vertices
		size_t maxSubdivision = static_cast<size_t>(pow(4.0, DETAIL_LEVEL_FIRST_PASS + 1));
		size_t passTwoSize = vertexCount * maxSubdivision * 2 * sizeof(float);
		g_passTwoVBO = gpu::buffer::CreateBuffer(passTwoSize);
	}

	// ---- Create VAOs ----
	{
		g_vaoPassOne = gpu::vao::CreateVertexArray(g_vec2VertexDescs);
		g_vaoPassTwo = gpu::vao::CreateVertexArray(g_vec2VertexDescs);
	}

	// ---- Create Pass One program (raw GL with transform feedback) ----
	{
		GLuint vs = CompileShaderGL(GL_VERTEX_SHADER,   passOneVertexSrc);
		GLuint gs = CompileShaderGL(GL_GEOMETRY_SHADER, passOneGeometrySrc);
		GLuint fs = CompileShaderGL(GL_FRAGMENT_SHADER, passOneFragmentSrc);

		g_passOneProgram = glCreateProgram();
		glAttachShader(g_passOneProgram, vs);
		glAttachShader(g_passOneProgram, gs);
		glAttachShader(g_passOneProgram, fs);

		static constexpr const char* TRANSFORM_VARYING = "v_vertex";
		glTransformFeedbackVaryings(g_passOneProgram, 1, &TRANSFORM_VARYING, GL_SEPARATE_ATTRIBS);

		glLinkProgram(g_passOneProgram);

		GLint status = 0;
		glGetProgramiv(g_passOneProgram, GL_LINK_STATUS, &status);
		assert(status && "PassOne program link failed");

		glDeleteShader(vs);
		glDeleteShader(gs);
		glDeleteShader(fs);

		g_halfDetailStepPassOneLocation      = glGetUniformLocation(g_passOneProgram, "u_halfDetailStep");
		g_detailLevelPassOneLocation          = glGetUniformLocation(g_passOneProgram, "u_detailLevel");
		g_fovRadiusPassOneLocation            = glGetUniformLocation(g_passOneProgram, "u_fovRadius");
		g_positionTextureSpacePassOneLocation = glGetUniformLocation(g_passOneProgram, "u_positionTextureSpace");
		g_leftNormalTextureSpacePassOneLocation  = glGetUniformLocation(g_passOneProgram, "u_leftNormalTextureSpace");
		g_rightNormalTextureSpacePassOneLocation = glGetUniformLocation(g_passOneProgram, "u_rightNormalTextureSpace");
		g_backNormalTextureSpacePassOneLocation  = glGetUniformLocation(g_passOneProgram, "u_backNormalTextureSpace");
	}

	// ---- Create Pass Two program (engine API) ----
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "PassTwo",
			.vertexShaderCode = passTwoVertexSrc,
			.fragmentShaderCode = passTwoFragmentSrc,
			.tessellationControlShaderCode = passTwoControlSrc,
			.tessellationEvaluationShaderCode = passTwoEvalSrc,
			.geometryShaderCode = passTwoGeometrySrc,
		};
		g_passTwoProgram = gpu::program::CreateShaderProgram(info);

		g_tmvpPassTwoLocation                 = gpu::program::GetUniformLocation(g_passTwoProgram, "u_tmvpMatrix");
		g_maxTessellationLevelPassTwoLocation = gpu::program::GetUniformLocation(g_passTwoProgram, "u_maxTessellationLevel");
		g_quadrantStepPassTwoLocation         = gpu::program::GetUniformLocation(g_passTwoProgram, "u_quadrantStep");
		g_positionTextureSpacePassTwoLocation = gpu::program::GetUniformLocation(g_passTwoProgram, "u_positionTextureSpace");
		g_heightMapTexturePassTwoLocation     = gpu::program::GetUniformLocation(g_passTwoProgram, "u_heightMapTexture");
		g_colorMapTexturePassTwoLocation      = gpu::program::GetUniformLocation(g_passTwoProgram, "u_colorMapTexture");
		g_normalMapTexturePassTwoLocation     = gpu::program::GetUniformLocation(g_passTwoProgram, "u_normalMapTexture");
		g_lightDirectionPassTwoLocation       = gpu::program::GetUniformLocation(g_passTwoProgram, "u_lightDirection");
	}

	// ---- Compute texture-to-world and world-to-texture matrices ----
	{
		g_textureToWorldMatrix = glm::mat4(1.0f);
		g_textureToWorldMatrix = glm::scale(g_textureToWorldMatrix, glm::vec3(HORIZ_SCALE, VERT_SCALE, HORIZ_SCALE));
		g_textureToWorldMatrix = glm::scale(g_textureToWorldMatrix, glm::vec3(1.0f, 1.0f, -1.0f));
		g_textureToWorldMatrix = glm::translate(g_textureToWorldMatrix, glm::vec3(-g_sMapExtend / 2.0f, 0.0f, -g_tMapExtend / 2.0f));

		glm::mat4 normal4x4 = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, -1.0f));
		glm::mat3 textureToWorldNormal = glm::mat3(normal4x4);

		g_worldToTextureMatrix = glm::inverse(g_textureToWorldMatrix);
		g_worldToTextureNormalMatrix = glm::transpose(textureToWorldNormal);
	}

	// ---- Set constant Pass One uniforms ----
	{
		glUseProgram(g_passOneProgram);

		float detailStep = powf(2.0f, static_cast<float>(g_overallMaxDetailLevel - MINIMUM_DETAIL_LEVEL));
		glUniform1f(g_halfDetailStepPassOneLocation, detailStep / 2.0f);
		glUniform1ui(g_detailLevelPassOneLocation, DETAIL_LEVEL_FIRST_PASS);
		glUniform1f(g_fovRadiusPassOneLocation, FOV_RADIUS / HORIZONTAL_PIXEL_SPACING * METERS_TO_VIRTUAL_WORLD_SCALE);

		glUseProgram(0);
	}

	// ---- Set constant Pass Two uniforms ----
	{
		gpu::program::BindShaderProgram(g_passTwoProgram);

		gpu::program::SetUniform(g_passTwoProgram, g_lightDirectionPassTwoLocation, g_lightDirection);
		gpu::program::SetUniform(g_passTwoProgram, g_maxTessellationLevelPassTwoLocation, g_overallMaxDetailLevel - (MINIMUM_DETAIL_LEVEL + DETAIL_LEVEL_FIRST_PASS));
		gpu::program::SetUniform(g_passTwoProgram, g_quadrantStepPassTwoLocation, QUADRANT_STEP);
		gpu::program::SetUniform(g_passTwoProgram, g_heightMapTexturePassTwoLocation, 0);
		gpu::program::SetUniform(g_passTwoProgram, g_colorMapTexturePassTwoLocation, 1);
		gpu::program::SetUniform(g_passTwoProgram, g_normalMapTexturePassTwoLocation, 2);
	}

	// ---- Transform feedback query ----
	{
		glGenQueries(1, &g_transformFeedbackQuery);
	}

	// ---- State defaults ----
	{
		depthState.depthTestEnable = true;
		depthState.depthWriteEnable = true;
	}

	// ---- Patch parameter ----
	{
		gpu::TessellationState tessState{ .patchControlPoints = 4 };
		gpu::cmd::SetState(tessState);
	}

	return true;
}
//=============================================================================
static void GameClose()
{
	glUseProgram(0);

	if (g_passOneProgram)
	{
		glDeleteProgram(g_passOneProgram);
		g_passOneProgram = 0;
	}

	g_passTwoProgram.reset();
	g_passOneVBO.reset();
	g_passTwoVBO.reset();
	g_vaoPassOne.reset();
	g_vaoPassTwo.reset();

	if (g_heightMapTexture)
	{
		glDeleteTextures(1, &g_heightMapTexture);
		g_heightMapTexture = 0;
	}
	if (g_normalMapTexture)
	{
		glDeleteTextures(1, &g_normalMapTexture);
		g_normalMapTexture = 0;
	}
	if (g_colorMapTexture)
	{
		glDeleteTextures(1, &g_colorMapTexture);
		g_colorMapTexture = 0;
	}
	if (g_transformFeedbackQuery)
	{
		glDeleteQueries(1, &g_transformFeedbackQuery);
		g_transformFeedbackQuery = 0;
	}
}
//=============================================================================
static void GameUpdate()
{
	float deltaTime = app::GetDeltaTime();

	if (g_animationOn)
	{
		g_angle += deltaTime;
	}

	// ---- Camera: orbit around terrain ----
	float angleRad = 2.0f * glm::pi<float>() * g_angle / TURN_DURATION;

	glm::vec3 cameraPos(
		-cosf(angleRad) * TURN_RADIUS * METERS_TO_VIRTUAL_WORLD_SCALE,
		4700.0f * METERS_TO_VIRTUAL_WORLD_SCALE,
		-sinf(angleRad) * TURN_RADIUS * METERS_TO_VIRTUAL_WORLD_SCALE);

	glm::vec3 cameraTarget(
		cameraPos.x + sinf(angleRad),
		cameraPos.y,
		cameraPos.z - cosf(angleRad));

	glm::vec3 cameraUp(0.0f, 1.0f, 0.0f);

	glm::mat4 viewMatrix = glm::lookAt(cameraPos, cameraTarget, cameraUp);

	// ---- Projection ----
	g_projectionMatrix = glm::perspective(glm::radians(60.0f), window::GetAspectRatio(), 1.0f, 1000000.0f);

	// ---- TMVP matrix: projection * view * textureToWorld ----
	glm::mat4 tmvpMatrix = g_projectionMatrix * viewMatrix * g_textureToWorldMatrix;

	// ---- Frustum planes in texture space ----
	glm::vec3 camDir = glm::normalize(cameraTarget - cameraPos);
	glm::vec3 camPos2D(cameraPos.x, 0.0f, cameraPos.z);

	// Camera position in texture space (2D xz plane)
	glm::vec4 posTexSpace = g_worldToTextureMatrix * glm::vec4(camPos2D, 1.0f);

	// Camera direction in texture space
	glm::vec3 dirTexSpace = glm::mat3(g_worldToTextureMatrix) * camDir;
	dirTexSpace = glm::normalize(dirTexSpace);

	// Frustum half-angle adjusted by aspect ratio
	float aspect = window::GetAspectRatio();
	float halfAngle = 60.0f * aspect / 2.0f;

	// Left normal
	glm::mat4 rotLeft = glm::rotate(glm::mat4(1.0f), glm::radians(halfAngle + 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 leftNormal = glm::mat3(rotLeft) * camDir;
	leftNormal = glm::normalize(g_worldToTextureNormalMatrix * leftNormal);

	// Right normal
	glm::mat4 rotRight = glm::rotate(glm::mat4(1.0f), glm::radians(-halfAngle - 90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 rightNormal = glm::mat3(rotRight) * camDir;
	rightNormal = glm::normalize(g_worldToTextureNormalMatrix * rightNormal);

	// Back normal
	glm::mat4 rotBack = glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 backNormal = glm::mat3(rotBack) * camDir;
	backNormal = glm::normalize(g_worldToTextureNormalMatrix * backNormal);

	// ---- Update Pass One uniforms ----
	{
		glUseProgram(g_passOneProgram);
		glUniform4fv(g_positionTextureSpacePassOneLocation, 1, &posTexSpace[0]);
		glUniform3fv(g_leftNormalTextureSpacePassOneLocation, 1, &leftNormal[0]);
		glUniform3fv(g_rightNormalTextureSpacePassOneLocation, 1, &rightNormal[0]);
		glUniform3fv(g_backNormalTextureSpacePassOneLocation, 1, &backNormal[0]);
		glUseProgram(0);
	}

	// ---- Update Pass Two uniforms ----
	{
		gpu::program::BindShaderProgram(g_passTwoProgram);
		gpu::program::SetUniform(g_passTwoProgram, g_tmvpPassTwoLocation, tmvpMatrix);
		gpu::program::SetUniform(g_passTwoProgram, g_positionTextureSpacePassTwoLocation, posTexSpace);
	}
}
//=============================================================================
static void GameFixedUpdate()
{}
//=============================================================================
static void GameRender()
{
	gpu::fbo::SwapchainRenderInfo swapchainRI = {};
	swapchainRI.colorLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.clearColorValue[0] = 0.4f;
	swapchainRI.clearColorValue[1] = 0.6f;
	swapchainRI.clearColorValue[2] = 0.8f;
	swapchainRI.clearColorValue[3] = 1.0f;
	swapchainRI.depthLoadOp = gpu::fbo::AttachmentLoadOp::Clear;
	swapchainRI.viewport.drawRect.offset = { 0, 0 };
	swapchainRI.viewport.drawRect.extent = { window::GetWidth(), window::GetHeight() };
	gpu::cmd::BeginDraw(swapchainRI, "MainFrame");
	{
		// ---- Pass One: Generate visible vertices via Transform Feedback ----
		glEnable(GL_RASTERIZER_DISCARD);

		glUseProgram(g_passOneProgram);

		GLuint vao1 = gpu::vao::Handle(g_vaoPassOne);
		GLuint buf1 = gpu::buffer::Handle(g_passOneVBO);
		GLuint buf2 = gpu::buffer::Handle(g_passTwoVBO);

		glBindVertexArray(vao1);
		glVertexArrayVertexBuffer(vao1, 0, buf1, 0, 2 * sizeof(float));
		glEnableVertexArrayAttrib(vao1, 0);
		glVertexArrayAttribFormat(vao1, 0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao1, 0, 0);

		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, buf2);

		glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, g_transformFeedbackQuery);
		glBeginTransformFeedback(GL_POINTS);

		glDrawArrays(GL_POINTS, 0, g_sNumPoints * g_tNumPoints);

		glEndTransformFeedback();
		glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
		glDisable(GL_RASTERIZER_DISCARD);
		glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

		glBindVertexArray(0);

		// Reset engine program tracking (Pass One used raw glUseProgram)
		gpu::program::BindShaderProgram(nullptr);

		// ---- Pass Two: Render terrain via tessellation ----
		gpu::cmd::SetState(depthState);
		gpu::cmd::SetState(defaultRasterState);
		gpu::cmd::BindShaderProgram(g_passTwoProgram);

		// Bind textures
		glBindTextureUnit(0, g_heightMapTexture);
		glBindTextureUnit(1, g_colorMapTexture);
		glBindTextureUnit(2, g_normalMapTexture);

		// Query number of primitives written
		GLuint primitivesWritten = 0;
		glGetQueryObjectuiv(g_transformFeedbackQuery, GL_QUERY_RESULT, &primitivesWritten);

		// Draw patches
		gpu::cmd::SetTopology(gpu::PrimitiveTopology::PatchList);

		GLuint vao2 = gpu::vao::Handle(g_vaoPassTwo);
		glBindVertexArray(vao2);
		glVertexArrayVertexBuffer(vao2, 0, buf2, 0, 2 * sizeof(float));
		glEnableVertexArrayAttrib(vao2, 0);
		glVertexArrayAttribFormat(vao2, 0, 2, GL_FLOAT, GL_FALSE, 0);
		glVertexArrayAttribBinding(vao2, 0, 0);

		gpu::cmd::Draw(primitivesWritten, 1, 0, 0);

		glBindVertexArray(0);
	}
	gpu::cmd::EndDraw();
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Simple Terrain");
	ImGui::Text("Example14: GPU LOD Terrain Rendering");
	ImGui::Text("Pass 1: Geometry culling + subdivision (Transform Feedback)");
	ImGui::Text("Pass 2: Tessellation + height/normal mapping");
	ImGui::Separator();
	ImGui::Text("Vertices (Pass 1): %u", g_sNumPoints * g_tNumPoints);
	ImGui::Text("Details level: %u", g_overallMaxDetailLevel);
	ImGui::Text("Animation: %s", g_animationOn ? "ON" : "OFF");
	ImGui::End();
}
//=============================================================================
void gpu007_simpleTerrain()
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
