#include "stdafx.h"
#include <glm/gtc/matrix_transform.hpp>
//=============================================================================
namespace
{
	//=======================================================================
	// Constants
	//=======================================================================
	static constexpr int   NUMBERWAVES        = 4;
	static constexpr int   WATER_PLANE_LENGTH = 128;
	static constexpr int   TEXTURE_SIZE       = 1024;
	static constexpr float OVERALL_STEEPNESS  = 0.2f;

	//=======================================================================
	// Wave parameter structs
	//=======================================================================
	struct WaveParameters final
	{
		float speed;
		float amplitude;
		float wavelength;
		float steepness;
	};

	struct WaveDirections final
	{
		float x;
		float z;
	};

	//=======================================================================
	// Shader sources
	//=======================================================================

	// ---- Water program (vertex displacement + reflection/refraction) ----
	const char* g_waterVertexSrc = R"(
#version 420 core

#define NUMBERWAVES 4

const float PI = 3.141592654;
const float G  = 9.81;

uniform mat4 u_projectionMatrix;
uniform mat4 u_viewMatrix;
uniform mat3 u_inverseViewNormalMatrix;

uniform float u_passedTime;
uniform float u_waterPlaneLength;

uniform vec4 u_waveParameters[NUMBERWAVES];
uniform vec2 u_waveDirections[NUMBERWAVES];

layout(location = 0) in vec4 a_vertex;

layout(location = 0) out vec3 v_incident;
layout(location = 1) out vec3 v_bitangent;
layout(location = 2) out vec3 v_normal;
layout(location = 3) out vec3 v_tangent;
layout(location = 4) out vec2 v_texCoord;

void main(void)
{
	vec4 finalVertex = a_vertex;

	vec3 finalBitangent = vec3(0.0);
	vec3 finalNormal    = vec3(0.0);
	vec3 finalTangent   = vec3(0.0);

	for (int i = 0; i < NUMBERWAVES; i++)
	{
		vec2  direction  = normalize(u_waveDirections[i]);
		float speed      = u_waveParameters[i].x;
		float amplitude  = u_waveParameters[i].y;
		float wavelength = u_waveParameters[i].z;
		float steepness  = u_waveParameters[i].w;

		float frequency = sqrt(G * 2.0 * PI / wavelength);
		float phase     = speed * frequency;
		float alpha     = frequency * dot(direction, a_vertex.xz) + phase * u_passedTime;

		finalVertex.x += steepness * amplitude * direction.x * cos(alpha);
		finalVertex.y += amplitude * sin(alpha);
		finalVertex.z += steepness * amplitude * direction.y * cos(alpha);
	}

	for (int i = 0; i < NUMBERWAVES; i++)
	{
		vec2  direction  = normalize(u_waveDirections[i]);
		float speed      = u_waveParameters[i].x;
		float amplitude  = u_waveParameters[i].y;
		float wavelength = u_waveParameters[i].z;
		float steepness  = u_waveParameters[i].w;

		float frequency = sqrt(G * 2.0 * PI / wavelength);
		float phase     = speed * frequency;
		float alpha     = frequency * dot(direction, finalVertex.xz) + phase * u_passedTime;

		finalBitangent.x += steepness * direction.x * direction.x * wavelength * amplitude * sin(alpha);
		finalBitangent.y += direction.x * wavelength * amplitude * cos(alpha);
		finalBitangent.z += steepness * direction.x * direction.y * wavelength * amplitude * sin(alpha);

		finalNormal.x += direction.x * wavelength * amplitude * cos(alpha);
		finalNormal.y += steepness * wavelength * amplitude * sin(alpha);
		finalNormal.z += direction.y * wavelength * amplitude * cos(alpha);

		finalTangent.x += steepness * direction.x * direction.y * wavelength * amplitude * sin(alpha);
		finalTangent.y += direction.y * wavelength * amplitude * cos(alpha);
		finalTangent.z += steepness * direction.y * direction.y * wavelength * amplitude * sin(alpha);
	}

	finalTangent.x = -finalTangent.x;
	finalTangent.z = 1.0 - finalTangent.z;
	finalTangent   = normalize(finalTangent);

	finalBitangent.x = 1.0 - finalBitangent.x;
	finalBitangent.z = -finalBitangent.z;
	finalBitangent   = normalize(finalBitangent);

	finalNormal.x = -finalNormal.x;
	finalNormal.y = 1.0 - finalNormal.y;
	finalNormal.z = -finalNormal.z;
	finalNormal   = normalize(finalNormal);

	v_bitangent = finalBitangent;
	v_normal    = finalNormal;
	v_tangent   = finalTangent;

	v_texCoord = vec2(
		clamp((finalVertex.x + u_waterPlaneLength * 0.5 - 0.5) / u_waterPlaneLength, 0.0, 1.0),
		clamp((-finalVertex.z + u_waterPlaneLength * 0.5 + 0.5) / u_waterPlaneLength, 0.0, 1.0));

	vec4 vertex = u_viewMatrix * finalVertex;

	v_incident = u_inverseViewNormalMatrix * vec3(vertex);

	gl_Position = u_projectionMatrix * vertex;
}
)";

	const char* g_waterFragmentSrc = R"(
#version 420 core

const float Eta = 0.15;

layout(binding = 0) uniform samplerCube u_cubemap;
layout(binding = 1) uniform sampler2D   u_waterTexture;

layout(location = 0) in vec3 v_incident;
layout(location = 1) in vec3 v_bitangent;
layout(location = 2) in vec3 v_normal;
layout(location = 3) in vec3 v_tangent;
layout(location = 4) in vec2 v_texCoord;

layout(location = 0) out vec4 fragColor;

vec3 textureToNormal(vec4 orgNormalColor)
{
	return normalize(vec3(
		clamp(orgNormalColor.r * 2.0 - 1.0, -1.0, 1.0),
		clamp(orgNormalColor.g * 2.0 - 1.0, -1.0, 1.0),
		clamp(orgNormalColor.b * 2.0 - 1.0, -1.0, 1.0)));
}

void main(void)
{
	vec3 objectNormal = textureToNormal(texture(u_waterTexture, v_texCoord));

	mat3 objectToWorldMatrix = mat3(
		normalize(v_bitangent),
		normalize(v_normal),
		normalize(v_tangent));

	vec3 worldNormal   = objectToWorldMatrix * objectNormal;
	vec3 worldIncident = normalize(v_incident);

	vec3 refraction = refract(worldIncident, worldNormal, Eta);
	vec3 reflection = reflect(worldIncident, worldNormal);

	vec4 refractionColor = texture(u_cubemap, refraction);
	vec4 reflectionColor = texture(u_cubemap, reflection);

	float fresnel = Eta + (1.0 - Eta) * pow(
		max(0.0, 1.0 - dot(-worldIncident, worldNormal)), 5.0);

	fragColor = mix(refractionColor, reflectionColor, fresnel);
}
)";

	// ---- WaterTexture program (generates normal map) ----
	const char* g_waterTextureVertexSrc = R"(
#version 420 core

uniform mat4 u_projectionMatrix;
uniform mat4 u_modelViewMatrix;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec2 a_texCoord;

layout(location = 0) out vec2 v_texCoord;

void main(void)
{
	v_texCoord = a_texCoord;

	gl_Position = u_projectionMatrix * u_modelViewMatrix * a_vertex;
}
)";

	const char* g_waterTextureFragmentSrc = R"(
#version 420 core

#define NUMBERWAVES 4

const float PI = 3.141592654;
const float G  = 9.81;

uniform float u_waterPlaneLength;
uniform float u_passedTime;

uniform vec4 u_waveParameters[NUMBERWAVES];
uniform vec2 u_waveDirections[NUMBERWAVES];

layout(location = 0) in vec2 v_texCoord;

layout(location = 0) out vec4 fragColor;

vec3 normalToTexture(vec3 orgNormal)
{
	return vec3(
		clamp(orgNormal.x * 0.5 + 0.5, 0.0, 1.0),
		clamp(orgNormal.y * 0.5 + 0.5, 0.0, 1.0),
		clamp(orgNormal.z * 0.5 + 0.5, 0.0, 1.0));
}

void main(void)
{
	vec3 vertex = vec3(
		v_texCoord.s * u_waterPlaneLength - u_waterPlaneLength / 2.0 + 0.5,
		0.0,
		-v_texCoord.t * u_waterPlaneLength + u_waterPlaneLength / 2.0 + 0.5);

	vec4 finalVertex = vec4(vertex, 1.0);

	vec3 finalNormal = vec3(0.0);

	for (int i = 0; i < NUMBERWAVES; i++)
	{
		vec2  direction  = normalize(u_waveDirections[i]);
		float speed      = u_waveParameters[i].x;
		float amplitude  = u_waveParameters[i].y;
		float wavelength = u_waveParameters[i].z;
		float steepness  = u_waveParameters[i].w;

		float frequency = sqrt(G * 2.0 * PI / wavelength);
		float phase     = speed * frequency;
		float alpha     = frequency * dot(direction, vertex.xz) + phase * u_passedTime;

		finalVertex.x += steepness * amplitude * direction.x * cos(alpha);
		finalVertex.y += amplitude * sin(alpha);
		finalVertex.z += steepness * amplitude * direction.y * cos(alpha);
	}

	for (int i = 0; i < NUMBERWAVES; i++)
	{
		vec2  direction  = normalize(u_waveDirections[i]);
		float speed      = u_waveParameters[i].x;
		float amplitude  = u_waveParameters[i].y;
		float wavelength = u_waveParameters[i].z;
		float steepness  = u_waveParameters[i].w;

		float frequency = sqrt(G * 2.0 * PI / wavelength);
		float phase     = speed * frequency;
		float alpha     = frequency * dot(direction, finalVertex.xz) + phase * u_passedTime;

		finalNormal.x += direction.x * wavelength * amplitude * cos(alpha);
		finalNormal.y += steepness * wavelength * amplitude * sin(alpha);
		finalNormal.z += direction.y * wavelength * amplitude * cos(alpha);
	}

	finalNormal.x = -finalNormal.x;
	finalNormal.y = 1.0 - finalNormal.y;
	finalNormal.z = -finalNormal.z;
	finalNormal   = normalize(finalNormal);

	fragColor = vec4(normalToTexture(finalNormal), 1.0);
}
)";

	// ---- Background program (sky sphere) ----
	const char* g_backgroundVertexSrc = R"(
#version 420 core

uniform mat4 u_projectionMatrix;
uniform mat4 u_modelViewMatrix;

layout(location = 0) in vec4 a_vertex;
layout(location = 1) in vec3 a_normal;

layout(location = 0) out vec3 v_ray;

void main(void)
{
	v_ray = normalize(a_vertex.xyz);

	gl_Position = u_projectionMatrix * u_modelViewMatrix * a_vertex;
}
)";

	const char* g_backgroundFragmentSrc = R"(
#version 420 core

layout(binding = 0) uniform samplerCube u_cubemap;

layout(location = 0) in vec3 v_ray;

layout(location = 0) out vec4 fragColor;

void main(void)
{
	fragColor = texture(u_cubemap, normalize(v_ray));
}
)";

	//=======================================================================
	// Geometry generation
	//=======================================================================
	[[nodiscard]] static std::vector<float> GenerateWaterVertices() noexcept
	{
		std::vector<float> verts;
		verts.reserve(static_cast<size_t>(WATER_PLANE_LENGTH) * WATER_PLANE_LENGTH * 4);
		for (int z = 0; z < WATER_PLANE_LENGTH; z++)
		{
			for (int x = 0; x < WATER_PLANE_LENGTH; x++)
			{
				verts.push_back(-static_cast<float>(WATER_PLANE_LENGTH) / 2.0f + 0.5f + static_cast<float>(x));
				verts.push_back(0.0f);
				verts.push_back(+static_cast<float>(WATER_PLANE_LENGTH) / 2.0f - 0.5f - static_cast<float>(z));
				verts.push_back(1.0f);
			}
		}
		return verts;
	}

	[[nodiscard]] static std::vector<uint32_t> GenerateWaterIndices() noexcept
	{
		std::vector<uint32_t> idx;
		idx.reserve(static_cast<size_t>(WATER_PLANE_LENGTH) * (WATER_PLANE_LENGTH - 1) * 2);
		for (int k = 0; k < WATER_PLANE_LENGTH - 1; k++)
		{
			for (int i = 0; i < WATER_PLANE_LENGTH; i++)
			{
				if (k % 2 == 0)
				{
					idx.push_back(static_cast<uint32_t>(i + (k + 1) * WATER_PLANE_LENGTH));
					idx.push_back(static_cast<uint32_t>(i + k * WATER_PLANE_LENGTH));
				}
				else
				{
					idx.push_back(static_cast<uint32_t>(WATER_PLANE_LENGTH - 1 - i + k * WATER_PLANE_LENGTH));
					idx.push_back(static_cast<uint32_t>(WATER_PLANE_LENGTH - 1 - i + (k + 1) * WATER_PLANE_LENGTH));
				}
			}
		}
		return idx;
	}

	[[nodiscard]] static void GeneratePlane(
		float size,
		std::vector<float>& outVertices,
		std::vector<float>& outTexCoords,
		std::vector<uint32_t>& outIndices) noexcept
	{
		float h = size;
		outVertices = { -h, -h, 0, 1,   h, -h, 0, 1,   h, h, 0, 1,  -h, h, 0, 1 };
		outTexCoords = { 0, 0,   1, 0,   1, 1,   0, 1 };
		outIndices = { 0, 1, 2,  0, 2, 3 };
	}

	[[nodiscard]] static void GenerateSphere(
		float radius, int segments,
		std::vector<float>& outVertices,
		std::vector<float>& outNormals,
		std::vector<uint32_t>& outIndices) noexcept
	{
		outVertices.clear();
		outNormals.clear();
		outIndices.clear();

		for (int j = 0; j <= segments; j++)
		{
			float theta = glm::pi<float>() * static_cast<float>(j) / static_cast<float>(segments);
			for (int i = 0; i <= segments; i++)
			{
				float phi = 2.0f * glm::pi<float>() * static_cast<float>(i) / static_cast<float>(segments);
				float st = sinf(theta);
				float ct = cosf(theta);
				float sp = sinf(phi);
				float cp = cosf(phi);

				outVertices.push_back(radius * st * cp);
				outVertices.push_back(radius * ct);
				outVertices.push_back(radius * st * sp);
				outVertices.push_back(1.0f);

				outNormals.push_back(st * cp);
				outNormals.push_back(ct);
				outNormals.push_back(st * sp);
			}
		}

		for (int j = 0; j < segments; j++)
		{
			for (int i = 0; i < segments; i++)
			{
				int i0 = j * (segments + 1) + i;
				int i1 = j * (segments + 1) + (i + 1);
				int i2 = (j + 1) * (segments + 1) + i;
				int i3 = (j + 1) * (segments + 1) + (i + 1);
				outIndices.push_back(static_cast<uint32_t>(i0));
				outIndices.push_back(static_cast<uint32_t>(i1));
				outIndices.push_back(static_cast<uint32_t>(i2));
				outIndices.push_back(static_cast<uint32_t>(i2));
				outIndices.push_back(static_cast<uint32_t>(i1));
				outIndices.push_back(static_cast<uint32_t>(i3));
			}
		}
	}

	//=======================================================================
	// Resources
	//=======================================================================

	// Programs
	gpu::program::ShaderProgramPtr g_waterProgram;
	int g_waterProjectionMatrixLoc            = -1;
	int g_waterViewMatrixLoc                  = -1;
	int g_waterInverseViewNormalMatrixLoc     = -1;
	int g_waterPassedTimeLoc                  = -1;
	int g_waterPlaneLengthLoc                 = -1;
	int g_waterWaveParametersLoc              = -1;
	int g_waterWaveDirectionsLoc              = -1;

	gpu::program::ShaderProgramPtr g_waterTextureProgram;
	int g_wtProjectionMatrixLoc               = -1;
	int g_wtModelViewMatrixLoc                = -1;
	int g_wtWaterPlaneLengthLoc               = -1;
	int g_wtPassedTimeLoc                     = -1;
	int g_wtWaveParametersLoc                 = -1;
	int g_wtWaveDirectionsLoc                 = -1;

	gpu::program::ShaderProgramPtr g_backgroundProgram;
	int g_bgProjectionMatrixLoc               = -1;
	int g_bgModelViewMatrixLoc                = -1;

	// Water VAO / VBO / IBO
	gpu::vao::VertexArrayPtr g_waterVAO;
	gpu::buffer::BufferPtr   g_waterVBO;
	gpu::buffer::BufferPtr   g_waterIBO;
	uint32_t g_waterIndexCount = 0;

	// WaterTexture VAO / VBO / IBO
	gpu::vao::VertexArrayPtr g_wtVAO;
	gpu::buffer::BufferPtr   g_wtVerticesVBO;
	gpu::buffer::BufferPtr   g_wtTexCoordsVBO;
	gpu::buffer::BufferPtr   g_wtIBO;
	uint32_t g_wtIndexCount = 0;

	// Background VAO / VBO / IBO
	gpu::vao::VertexArrayPtr g_bgVAO;
	gpu::buffer::BufferPtr   g_bgVerticesVBO;
	gpu::buffer::BufferPtr   g_bgNormalsVBO;
	gpu::buffer::BufferPtr   g_bgIBO;
	uint32_t g_bgIndexCount = 0;

	// Cubemap
	gpu::texture::TexturePtr g_cubemapTexture;
	gpu::texture::SamplerPtr g_cubemapSampler;

	// WaterTexture FBO
	gpu::texture::TexturePtr  g_mirrorTexture;
	gpu::texture::TexturePtr  g_depthTexture;
	gpu::fbo::FramebufferPtr  g_waterFBO;
	gpu::texture::SamplerPtr  g_mirrorSampler;

	// Matrices & state
	glm::mat4 g_projectionMatrix(1.0f);
	glm::mat4 g_viewMatrix(1.0f);
	glm::mat3 g_inverseViewNormalMatrix(1.0f);

	float g_passedTime = 0.0f;
	float g_angle = 0.0f;

	gpu::DepthState g_depthState;
	gpu::RasterizationState g_rasterState;

	// VAO descriptions
	static const auto g_waterVertexDesc = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32A32_FLOAT,
			.offset   = 0,
		},
	};

	static const auto g_wtVertexDesc = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32A32_FLOAT,
			.offset   = 0,
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 1,
			.binding  = 1,
			.format   = gpu::Format::R32G32_FLOAT,
			.offset   = 0,
		},
	};

	static const auto g_bgVertexDesc = std::vector{
		gpu::vao::VertexInputBindingDescription{
			.location = 0,
			.binding  = 0,
			.format   = gpu::Format::R32G32B32A32_FLOAT,
			.offset   = 0,
		},
		gpu::vao::VertexInputBindingDescription{
			.location = 1,
			.binding  = 1,
			.format   = gpu::Format::R32G32B32_FLOAT,
			.offset   = 0,
		},
	};

	static constexpr const char* CUBEMAP_FILES[] = {
		"data/textures/water_cubemap/water_pos_x.tga",
		"data/textures/water_cubemap/water_neg_x.tga",
		"data/textures/water_cubemap/water_pos_y.tga",
		"data/textures/water_cubemap/water_neg_y.tga",
		"data/textures/water_cubemap/water_pos_z.tga",
		"data/textures/water_cubemap/water_neg_z.tga",
	};
} // anonymous namespace
//=============================================================================
static bool GameInit()
{
	// ---- Create programs ----
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "Water",
			.vertexShaderCode = g_waterVertexSrc,
			.fragmentShaderCode = g_waterFragmentSrc,
		};
		g_waterProgram = gpu::program::CreateShaderProgram(info);

		g_waterProjectionMatrixLoc        = gpu::program::GetUniformLocation(g_waterProgram, "u_projectionMatrix");
		g_waterViewMatrixLoc              = gpu::program::GetUniformLocation(g_waterProgram, "u_viewMatrix");
		g_waterInverseViewNormalMatrixLoc = gpu::program::GetUniformLocation(g_waterProgram, "u_inverseViewNormalMatrix");
		g_waterPassedTimeLoc              = gpu::program::GetUniformLocation(g_waterProgram, "u_passedTime");
		g_waterPlaneLengthLoc             = gpu::program::GetUniformLocation(g_waterProgram, "u_waterPlaneLength");
		g_waterWaveParametersLoc          = gpu::program::GetUniformLocation(g_waterProgram, "u_waveParameters");
		g_waterWaveDirectionsLoc          = gpu::program::GetUniformLocation(g_waterProgram, "u_waveDirections");
	}

	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "WaterTexture",
			.vertexShaderCode = g_waterTextureVertexSrc,
			.fragmentShaderCode = g_waterTextureFragmentSrc,
		};
		g_waterTextureProgram = gpu::program::CreateShaderProgram(info);

		g_wtProjectionMatrixLoc = gpu::program::GetUniformLocation(g_waterTextureProgram, "u_projectionMatrix");
		g_wtModelViewMatrixLoc  = gpu::program::GetUniformLocation(g_waterTextureProgram, "u_modelViewMatrix");
		g_wtWaterPlaneLengthLoc = gpu::program::GetUniformLocation(g_waterTextureProgram, "u_waterPlaneLength");
		g_wtPassedTimeLoc       = gpu::program::GetUniformLocation(g_waterTextureProgram, "u_passedTime");
		g_wtWaveParametersLoc   = gpu::program::GetUniformLocation(g_waterTextureProgram, "u_waveParameters");
		g_wtWaveDirectionsLoc   = gpu::program::GetUniformLocation(g_waterTextureProgram, "u_waveDirections");
	}

	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "Background",
			.vertexShaderCode = g_backgroundVertexSrc,
			.fragmentShaderCode = g_backgroundFragmentSrc,
		};
		g_backgroundProgram = gpu::program::CreateShaderProgram(info);

		g_bgProjectionMatrixLoc = gpu::program::GetUniformLocation(g_backgroundProgram, "u_projectionMatrix");
		g_bgModelViewMatrixLoc  = gpu::program::GetUniformLocation(g_backgroundProgram, "u_modelViewMatrix");
	}

	// ---- Generate water plane geometry ----
	std::vector<float> waterVertices = GenerateWaterVertices();
	std::vector<uint32_t> waterIndices = GenerateWaterIndices();
	g_waterIndexCount = static_cast<uint32_t>(waterIndices.size());

	// ---- Generate water texture plane geometry ----
	std::vector<float> wtVertices;
	std::vector<float> wtTexCoords;
	std::vector<uint32_t> wtIndices;
	GeneratePlane(static_cast<float>(TEXTURE_SIZE) / 2.0f, wtVertices, wtTexCoords, wtIndices);
	g_wtIndexCount = static_cast<uint32_t>(wtIndices.size());

	// ---- Generate sphere geometry for background ----
	float sphereRadius = static_cast<float>(WATER_PLANE_LENGTH) / 2.0f + 0.5f;
	std::vector<float> bgVertices;
	std::vector<float> bgNormals;
	std::vector<uint32_t> bgIndices;
	GenerateSphere(sphereRadius, 32, bgVertices, bgNormals, bgIndices);
	g_bgIndexCount = static_cast<uint32_t>(bgIndices.size());

	// ---- Create buffers ----
	g_waterVBO = gpu::buffer::CreateBuffer(waterVertices.data(),
		waterVertices.size() * sizeof(float));
	g_waterIBO = gpu::buffer::CreateBuffer(waterIndices.data(),
		waterIndices.size() * sizeof(uint32_t));

	g_wtVerticesVBO = gpu::buffer::CreateBuffer(wtVertices.data(),
		wtVertices.size() * sizeof(float));
	g_wtTexCoordsVBO = gpu::buffer::CreateBuffer(wtTexCoords.data(),
		wtTexCoords.size() * sizeof(float));
	g_wtIBO = gpu::buffer::CreateBuffer(wtIndices.data(),
		wtIndices.size() * sizeof(uint32_t));

	g_bgVerticesVBO = gpu::buffer::CreateBuffer(bgVertices.data(),
		bgVertices.size() * sizeof(float));
	g_bgNormalsVBO = gpu::buffer::CreateBuffer(bgNormals.data(),
		bgNormals.size() * sizeof(float));
	g_bgIBO = gpu::buffer::CreateBuffer(bgIndices.data(),
		bgIndices.size() * sizeof(uint32_t));

	// ---- Create VAOs ----
	g_waterVAO = gpu::vao::CreateVertexArray(g_waterVertexDesc);
	g_wtVAO = gpu::vao::CreateVertexArray(g_wtVertexDesc);
	g_bgVAO = gpu::vao::CreateVertexArray(g_bgVertexDesc);

	// ---- Load cubemap ----
	{
		int imgW = 0, imgH = 0;
		stbi_set_flip_vertically_on_load(false);
		auto* firstPixels = stbi_load(CUBEMAP_FILES[0], &imgW, &imgH, nullptr, 4);
		assert(firstPixels && "Failed to load cubemap face 0");

		gpu::texture::TextureCreateInfo cubemapInfo{
			.imageType = gpu::ImageType::TextureCubemap,
			.format = gpu::Format::R8G8B8A8_UNORM,
			.extent = {static_cast<uint32_t>(imgW), static_cast<uint32_t>(imgH), 1},
			.mipLevels = 1,
			.arrayLayers = 6,
			.sampleCount = gpu::SampleCount::Samples1,
		};
		g_cubemapTexture = gpu::texture::CreateTexture(cubemapInfo, "water_cubemap");

		gpu::texture::UpdateImage(g_cubemapTexture, {
			.level = 0, .offset = {0, 0, 0},
			.extent = {static_cast<uint32_t>(imgW), static_cast<uint32_t>(imgH), 1},
			.format = gpu::UploadFormat::RGBA, .type = gpu::UploadType::UBYTE,
			.pixels = firstPixels,
		});
		stbi_image_free(firstPixels);

		for (unsigned i = 1; i < 6; ++i)
		{
			int w = 0, h = 0;
			auto* pixels = stbi_load(CUBEMAP_FILES[i], &w, &h, nullptr, 4);
			assert(pixels && w == imgW && h == imgH && "Cubemap face size mismatch");
			gpu::texture::UpdateImage(g_cubemapTexture, {
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
		g_cubemapSampler = gpu::texture::CreateSampler(ss);
	}

	// ---- Mirror texture sampler (matches original: linear, repeat) ----
	{
		gpu::texture::SamplerState ms;
		ms.minFilter = gpu::Filter::Linear;
		ms.magFilter = gpu::Filter::Linear;
		ms.addressModeU = gpu::AddressMode::Repeat;
		ms.addressModeV = gpu::AddressMode::Repeat;
		ms.addressModeW = gpu::AddressMode::Repeat;
		g_mirrorSampler = gpu::texture::CreateSampler(ms);
	}

	// ---- Create water texture FBO ----
	{
		g_mirrorTexture = gpu::texture::CreateTexture2D(
			{ TEXTURE_SIZE, TEXTURE_SIZE }, gpu::Format::R8G8B8A8_UNORM, "mirrorTexture");

		g_depthTexture = gpu::texture::CreateTexture2D(
			{ TEXTURE_SIZE, TEXTURE_SIZE }, gpu::Format::D32_FLOAT, "mirrorDepth");

		gpu::fbo::FramebufferCreateInfo fboInfo{};
		fboInfo.colorAttachments.push_back({
			.texture = g_mirrorTexture,
			.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
			.clearValue = {0.0f, 0.0f, 0.0f, 1.0f},
		});
		fboInfo.depthAttachment = gpu::fbo::RenderDepthStencilAttachment{
			.texture = g_depthTexture,
			.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
			.clearValue = {1.0f, 0},
		};
		g_waterFBO = gpu::fbo::CreateFramebuffer(fboInfo);
	}

	// ---- Set constant uniforms ----
	{
		gpu::program::BindShaderProgram(g_waterProgram);
		gpu::program::SetUniform(g_waterProgram, g_waterPlaneLengthLoc,
			static_cast<float>(WATER_PLANE_LENGTH));
	}

	{
		gpu::program::BindShaderProgram(g_backgroundProgram);
	}

	{
		gpu::program::BindShaderProgram(g_waterTextureProgram);
		gpu::program::SetUniform(g_waterTextureProgram, g_wtWaterPlaneLengthLoc,
			static_cast<float>(WATER_PLANE_LENGTH));
	}

	gpu::program::BindShaderProgram(nullptr);

	// ---- State defaults ----
	g_depthState.depthTestEnable = true;
	g_depthState.depthWriteEnable = true;
	g_rasterState.cullMode = gpu::CullMode::None;

	return true;
}
//=============================================================================
static void GameClose()
{
	g_waterProgram.reset();
	g_waterTextureProgram.reset();
	g_backgroundProgram.reset();

	g_waterVAO.reset();
	g_waterVBO.reset();
	g_waterIBO.reset();

	g_wtVAO.reset();
	g_wtVerticesVBO.reset();
	g_wtTexCoordsVBO.reset();
	g_wtIBO.reset();

	g_bgVAO.reset();
	g_bgVerticesVBO.reset();
	g_bgNormalsVBO.reset();
	g_bgIBO.reset();

	g_cubemapTexture.reset();
	g_cubemapSampler.reset();

	g_mirrorSampler.reset();
	g_mirrorTexture.reset();
	g_depthTexture.reset();
	g_waterFBO.reset();
}
//=============================================================================
static void GameUpdate()
{
	float deltaTime = app::GetDeltaTime();

	g_passedTime += deltaTime;
	g_angle += 2.0f * glm::pi<float>() / 120.0f * deltaTime;

	// ---- View matrix ----
	g_viewMatrix = glm::lookAt(
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.5f * sinf(g_angle), 1.0f, -0.5f * cosf(g_angle)),
		glm::vec3(0.0f, 1.0f, 0.0f));

	// Inverse view normal matrix (transpose of inverse upper-left 3x3)
	glm::mat4 inverseView = glm::inverse(g_viewMatrix);
	g_inverseViewNormalMatrix = glm::mat3(inverseView);

	// ---- Projection matrix ----
	g_projectionMatrix = glm::perspective(
		glm::radians(40.0f), window::GetAspectRatio(), 1.0f, 1000.0f);

	// ---- Update Water uniform(s) ----
	{
		gpu::program::BindShaderProgram(g_waterProgram);
		gpu::program::SetUniform(g_waterProgram, g_waterPassedTimeLoc, g_passedTime);
	}

	// ---- Update WaterTexture uniform(s) ----
	{
		gpu::program::BindShaderProgram(g_waterTextureProgram);
		gpu::program::SetUniform(g_waterTextureProgram, g_wtPassedTimeLoc, g_passedTime);
	}
}
//=============================================================================
static void GameFixedUpdate()
{}
//=============================================================================
static void RenderBackground()
{
	gpu::cmd::SetState(g_depthState);
	gpu::cmd::SetState(g_rasterState);
	gpu::cmd::BindShaderProgram(g_backgroundProgram);

	gpu::program::SetUniform(g_backgroundProgram, g_bgProjectionMatrixLoc, g_projectionMatrix);
	gpu::program::SetUniform(g_backgroundProgram, g_bgModelViewMatrixLoc, g_viewMatrix);

	gpu::cmd::BindSampledImage(0, g_cubemapTexture, g_cubemapSampler);

	gpu::cmd::BindVertexArray(g_bgVAO);
	gpu::cmd::BindVertexBuffer(g_bgVAO, 0, g_bgVerticesVBO, 0, 4 * sizeof(float));
	gpu::cmd::BindVertexBuffer(g_bgVAO, 1, g_bgNormalsVBO, 0, 3 * sizeof(float));
	gpu::cmd::BindIndexBuffer(g_bgVAO, g_bgIBO, gpu::IndexType::UnsignedInt);

	gpu::cmd::SetTopology(gpu::PrimitiveTopology::TriangleList);

	// Cull front faces to render inside of sphere (skybox effect)
	gpu::RasterizationState bgRaster = g_rasterState;
	bgRaster.cullMode = gpu::CullMode::Front;
	gpu::cmd::SetState(bgRaster);

	gpu::cmd::DrawIndexed(g_bgIndexCount, 1, 0, 0, 0);
}
//=============================================================================
static void RenderWaterTexture()
{
	gpu::cmd::SetState(g_depthState);
	gpu::cmd::BindShaderProgram(g_waterTextureProgram);

	// Orthographic projection
	glm::mat4 wtProjection = glm::ortho(
		-static_cast<float>(TEXTURE_SIZE) / 2.0f,
		static_cast<float>(TEXTURE_SIZE) / 2.0f,
		-static_cast<float>(TEXTURE_SIZE) / 2.0f,
		static_cast<float>(TEXTURE_SIZE) / 2.0f,
		1.0f, 100.0f);

	// ModelView: camera at (0,0,5) looking at origin
	glm::mat4 wtModelView = glm::lookAt(
		glm::vec3(0.0f, 0.0f, 5.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	gpu::program::SetUniform(g_waterTextureProgram, g_wtProjectionMatrixLoc, wtProjection);
	gpu::program::SetUniform(g_waterTextureProgram, g_wtModelViewMatrixLoc, wtModelView);

	// Set wave parameters (same as original renderWaterTexture.c)
	WaveParameters waveParams[NUMBERWAVES]{};
	WaveDirections waveDirs[NUMBERWAVES]{};

	waveParams[0] = { 0.05f, 0.02f, 0.3f, OVERALL_STEEPNESS / (0.3f * 0.02f * NUMBERWAVES) };
	waveDirs[0]   = { +1.0f, +1.5f };

	waveParams[1] = { 0.1f,  0.01f, 0.4f, OVERALL_STEEPNESS / (0.4f * 0.01f * NUMBERWAVES) };
	waveDirs[1]   = { +0.8f, +0.2f };

	waveParams[2] = { 0.04f, 0.035f, 0.1f, OVERALL_STEEPNESS / (0.4f * 0.01f * NUMBERWAVES) };
	waveDirs[2]   = { -0.2f, -0.1f };

	waveParams[3] = { 0.05f, 0.007f, 0.2f, OVERALL_STEEPNESS / (0.4f * 0.01f * NUMBERWAVES) };
	waveDirs[3]   = { -0.4f, -0.3f };

	gpu::program::SetUniform(g_waterTextureProgram, g_wtWaveParametersLoc,
		std::vector<glm::vec4>{
			glm::vec4(waveParams[0].speed, waveParams[0].amplitude, waveParams[0].wavelength, waveParams[0].steepness),
			glm::vec4(waveParams[1].speed, waveParams[1].amplitude, waveParams[1].wavelength, waveParams[1].steepness),
			glm::vec4(waveParams[2].speed, waveParams[2].amplitude, waveParams[2].wavelength, waveParams[2].steepness),
			glm::vec4(waveParams[3].speed, waveParams[3].amplitude, waveParams[3].wavelength, waveParams[3].steepness),
		});
	gpu::program::SetUniform(g_waterTextureProgram, g_wtWaveDirectionsLoc,
		std::vector<glm::vec2>{
			glm::vec2(waveDirs[0].x, waveDirs[0].z),
			glm::vec2(waveDirs[1].x, waveDirs[1].z),
			glm::vec2(waveDirs[2].x, waveDirs[2].z),
			glm::vec2(waveDirs[3].x, waveDirs[3].z),
		});

	gpu::cmd::BindVertexArray(g_wtVAO);
	gpu::cmd::BindVertexBuffer(g_wtVAO, 0, g_wtVerticesVBO, 0, 4 * sizeof(float));
	gpu::cmd::BindVertexBuffer(g_wtVAO, 1, g_wtTexCoordsVBO, 0, 2 * sizeof(float));
	gpu::cmd::BindIndexBuffer(g_wtVAO, g_wtIBO, gpu::IndexType::UnsignedInt);

	gpu::cmd::SetTopology(gpu::PrimitiveTopology::TriangleList);

	gpu::RasterizationState wtRaster = g_rasterState;
	wtRaster.frontFace = gpu::FrontFace::CounterClockWise;
	wtRaster.cullMode = gpu::CullMode::Back;
	gpu::cmd::SetState(wtRaster);

	gpu::cmd::DrawIndexed(g_wtIndexCount, 1, 0, 0, 0);
}
//=============================================================================
static void RenderWater()
{
	gpu::cmd::SetState(g_depthState);
	gpu::cmd::BindShaderProgram(g_waterProgram);

	gpu::program::SetUniform(g_waterProgram, g_waterProjectionMatrixLoc, g_projectionMatrix);
	gpu::program::SetUniform(g_waterProgram, g_waterViewMatrixLoc, g_viewMatrix);
	gpu::program::SetUniform(g_waterProgram, g_waterInverseViewNormalMatrixLoc, g_inverseViewNormalMatrix);

	// Set wave parameters (same as original main.c renderWater)
	WaveParameters waveParams[NUMBERWAVES]{};
	WaveDirections waveDirs[NUMBERWAVES]{};

	waveParams[0] = { 1.0f, 0.01f, 4.0f, OVERALL_STEEPNESS / (4.0f * 0.01f * NUMBERWAVES) };
	waveDirs[0]   = { +1.0f, +1.0f };

	waveParams[1] = { 0.5f, 0.02f, 3.0f, OVERALL_STEEPNESS / (3.0f * 0.02f * NUMBERWAVES) };
	waveDirs[1]   = { +1.0f, +0.0f };

	waveParams[2] = { 0.1f, 0.015f, 2.0f, OVERALL_STEEPNESS / (3.0f * 0.02f * NUMBERWAVES) };
	waveDirs[2]   = { -0.1f, -0.2f };

	waveParams[3] = { 1.1f, 0.008f, 1.0f, OVERALL_STEEPNESS / (3.0f * 0.02f * NUMBERWAVES) };
	waveDirs[3]   = { -0.2f, -0.1f };

	gpu::program::SetUniform(g_waterProgram, g_waterWaveParametersLoc,
		std::vector<glm::vec4>{
			glm::vec4(waveParams[0].speed, waveParams[0].amplitude, waveParams[0].wavelength, waveParams[0].steepness),
			glm::vec4(waveParams[1].speed, waveParams[1].amplitude, waveParams[1].wavelength, waveParams[1].steepness),
			glm::vec4(waveParams[2].speed, waveParams[2].amplitude, waveParams[2].wavelength, waveParams[2].steepness),
			glm::vec4(waveParams[3].speed, waveParams[3].amplitude, waveParams[3].wavelength, waveParams[3].steepness),
		});
	gpu::program::SetUniform(g_waterProgram, g_waterWaveDirectionsLoc,
		std::vector<glm::vec2>{
			glm::vec2(waveDirs[0].x, waveDirs[0].z),
			glm::vec2(waveDirs[1].x, waveDirs[1].z),
			glm::vec2(waveDirs[2].x, waveDirs[2].z),
			glm::vec2(waveDirs[3].x, waveDirs[3].z),
		});

	gpu::cmd::BindSampledImage(0, g_cubemapTexture, g_cubemapSampler);
	gpu::cmd::BindSampledImage(1, g_mirrorTexture, g_mirrorSampler);

	gpu::cmd::BindVertexArray(g_waterVAO);
	gpu::cmd::BindVertexBuffer(g_waterVAO, 0, g_waterVBO, 0, 4 * sizeof(float));
	gpu::cmd::BindIndexBuffer(g_waterVAO, g_waterIBO, gpu::IndexType::UnsignedInt);

	gpu::cmd::SetTopology(gpu::PrimitiveTopology::TriangleStrip);

	gpu::RasterizationState waterRaster = g_rasterState;
	waterRaster.frontFace = gpu::FrontFace::CounterClockWise;
	waterRaster.cullMode = gpu::CullMode::Back;
	gpu::cmd::SetState(waterRaster);

	gpu::cmd::DrawIndexed(g_waterIndexCount, 1, 0, 0, 0);
}
//=============================================================================
static void GameRender()
{
	// ---- Pass 1: Background (sky sphere) ----
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
		gpu::cmd::BeginDraw(swapchainRI, "Background");
		{
			RenderBackground();
		}
		gpu::cmd::EndDraw();
	}

	// ---- Pass 2: WaterTexture (normal map to FBO) ----
	{
		gpu::cmd::BeginDraw(g_waterFBO, "WaterTexture");
		{
			RenderWaterTexture();
		}
		gpu::cmd::EndDraw();
	}

	// ---- Pass 3: Water (main water on top) ----
	{
		gpu::fbo::SwapchainRenderInfo swapchainRI = {};
		swapchainRI.colorLoadOp = gpu::fbo::AttachmentLoadOp::Load;
		swapchainRI.depthLoadOp = gpu::fbo::AttachmentLoadOp::Load;
		swapchainRI.viewport.drawRect.offset = { 0, 0 };
		swapchainRI.viewport.drawRect.extent = { window::GetWidth(), window::GetHeight() };
		gpu::cmd::BeginDraw(swapchainRI, "Water");
		{
			RenderWater();
		}
		gpu::cmd::EndDraw();
	}
}
//=============================================================================
static void GameRenderUI()
{
	ImGui::Begin("Water Simulation");
	ImGui::Text("Example15: GPU Gems Water");
	ImGui::Text("3 passes: Background | Normal Map | Water");
	ImGui::Separator();
	ImGui::Text("Waves: %d", NUMBERWAVES);
	ImGui::Text("Time: %.2f", g_passedTime);
	ImGui::End();
}
//=============================================================================
void gpu008_water()
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
