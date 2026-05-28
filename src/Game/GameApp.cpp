#include "stdafx.h"
//=============================================================================
namespace
{
	const char* blinnPhongVert = R"(
#version 460 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

layout(std430, binding = 6) readonly buffer InstanceBuffer {
	mat4 models[];
} u_instanceData;

uniform bool  u_isInstanced;
uniform mat4  u_model;
uniform mat4  u_view;
uniform mat4  u_projection;
uniform mat3  u_normalMatrix;

layout(location = 0) out vec3 v_worldPos;
layout(location = 1) out vec3 v_worldNormal;
layout(location = 2) out vec2 v_texcoord;

void main()
{
	mat4 model = u_isInstanced ? u_instanceData.models[gl_InstanceID] : u_model;
	mat3 normalMatrix = u_isInstanced ? transpose(inverse(mat3(model))) : u_normalMatrix;

	vec4 worldPos = model * vec4(a_position, 1.0);
	v_worldPos    = worldPos.xyz;
	v_worldNormal = normalize(normalMatrix * a_normal);
	v_texcoord    = a_texcoord;

	gl_Position = u_projection * u_view * worldPos;
}
)";

	const char* blinnPhongFrag = R"(
#version 460 core

#define MAX_LIGHTS 16

struct LightData {
	vec4  positionOrDirection; // w=0 directional, w=1 point, w=2 spot
	vec3  color;
	float intensity;
	vec3  attenuation;         // constant, linear, quadratic
	float radius;
	vec3  spotDirection;
	float innerCutoff;         // cos(innerAngle)
	float outerCutoff;         // cos(outerAngle)
	int   type;                // 0=directional, 1=point, 2=spot
	bool  castShadow;
	float shadowBias;
	mat4  lightSpaceMatrix;
};

layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec2 v_texcoord;

layout(std140, binding = 4) uniform LightBlock {
	int        u_lightCount;
	LightData  u_lights[MAX_LIGHTS];
};
uniform vec3       u_cameraPos;

// Material uniforms
uniform vec3  u_albedoColor;
uniform vec3  u_specularColor;
uniform vec3  u_ambientColor;
uniform float u_shininess;
uniform float u_opacity;

// Texture uniforms (sampler handles, 0 = no texture)
uniform bool       u_hasAlbedoMap;
uniform bool       u_hasNormalMap;
uniform bool       u_hasSpecularMap;
uniform bool       u_hasEmissiveMap;
layout(binding = 0) uniform sampler2D u_albedoMap;
layout(binding = 1) uniform sampler2D u_normalMap;
layout(binding = 2) uniform sampler2D u_specularMap;
layout(binding = 3) uniform sampler2D u_emissiveMap;

// Shadow
uniform bool       u_receiveShadow;
uniform sampler2D  u_shadowMap;
uniform float      u_shadowMapSize;

// Point light shadow
uniform samplerCube u_pointShadowMap;
uniform float       u_pointShadowMapSize;

layout(location = 0) out vec4 o_color;

float ShadowCalculation(vec4 lightSpacePos, float bias)
{
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	float currentDepth = projCoords.z - bias;
	if (currentDepth > 1.0) return 1.0;

	float shadow = 0.0;
	float texelSize = 1.0 / u_shadowMapSize;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(u_shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth < pcfDepth ? 1.0 : 0.0;
		}
	}
	return shadow / 9.0;
}

float PointShadowCalculation(vec3 fragToLight, float bias, float farPlane)
{
	float closestDepth = texture(u_pointShadowMap, normalize(fragToLight)).r;
	float currentDepth = length(fragToLight) / farPlane;
	return (currentDepth - bias) < closestDepth ? 1.0 : 0.0;
}

vec3 CalcDirectionalLight(LightData light, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3 L = normalize(-light.spotDirection);
	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	float shadow = 1.0;
	if (light.castShadow && u_receiveShadow)
	{
		vec4 lightSpacePos = light.lightSpaceMatrix * vec4(v_worldPos, 1.0);
		shadow = ShadowCalculation(lightSpacePos, light.shadowBias);
	}

	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient * albedo + (diffuse + spec) * shadow) * light.intensity;
}

vec3 CalcPointLight(LightData light, vec3 fragPos, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3  L       = light.positionOrDirection.xyz - fragPos;
	float dist    = length(L);
	if (dist > light.radius) return vec3(0.0);
	L /= dist;

	float atten = 1.0 / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);
	float fade  = 1.0 - (dist * dist) / (light.radius * light.radius);
	atten *= fade * fade;

	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	float shadow = 1.0;
	if (light.castShadow && u_receiveShadow)
	{
		vec3 fragToLight = fragPos - light.positionOrDirection.xyz;
		shadow = PointShadowCalculation(fragToLight, light.shadowBias, light.radius);
	}

	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient * albedo + (diffuse + spec) * shadow) * light.intensity * atten;
}

vec3 CalcSpotLight(LightData light, vec3 fragPos, vec3 N, vec3 V, vec3 albedo, vec3 specular, vec3 ambient, float shininess)
{
	vec3  L       = light.positionOrDirection.xyz - fragPos;
	float dist    = length(L);
	if (dist > light.radius) return vec3(0.0);
	L /= dist;

	float theta     = dot(L, normalize(-light.spotDirection));
	float epsilon   = light.innerCutoff - light.outerCutoff;
	float spot      = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

	float atten = spot / (light.attenuation.x + light.attenuation.y * dist + light.attenuation.z * dist * dist);
	float fade  = 1.0 - (dist * dist) / (light.radius * light.radius);
	atten *= fade * fade;

	vec3 H = normalize(L + V);

	float NdotL = max(dot(N, L), 0.0);
	float NdotH = max(dot(N, H), 0.0);

	vec3 diffuse  = light.color * albedo * NdotL;
	vec3 spec     = light.color * specular * pow(NdotH, shininess);

	return (ambient * albedo + diffuse + spec) * light.intensity * atten;
}

void main()
{
    //// DEBUG: show shadow map UV / depth on the plane
    //if (u_lightCount > 0 && u_lights[0].castShadow)
    //{
    //    vec4 lsp = u_lights[0].lightSpaceMatrix * vec4(v_worldPos, 1.0);
    //    vec3 p = lsp.xyz / lsp.w;
    //    p = p * 0.5 + 0.5;
    //    float sm = texture(u_shadowMap, p.xy).r;
    //    o_color = vec4(p.xy, sm, 1.0);
    //    return;
    //}

	vec3 albedo   = u_albedoColor;
	vec3 specular = u_specularColor;
	vec3 ambient  = u_ambientColor;
	vec3 normal   = normalize(v_worldNormal);

	if (u_hasAlbedoMap)   albedo   *= texture(u_albedoMap,   v_texcoord).rgb;
	if (u_hasSpecularMap) specular *= texture(u_specularMap, v_texcoord).rgb;
	if (u_hasNormalMap)
	{
		vec3 n = texture(u_normalMap, v_texcoord).xyz * 2.0 - 1.0;
		// Simple tangent-space normal: derive TBN from screen-space derivatives
		vec3 dx = dFdx(v_worldPos);
		vec3 dy = dFdy(v_worldPos);
		vec3 T  = normalize(dx * n.x + dy * n.y);
		vec3 B  = normalize(cross(normal, T));
		vec3 N  = normalize(cross(T, B));
		mat3 TBN = mat3(T, B, N);
		normal = normalize(TBN * n);
	}

	vec3 V = normalize(u_cameraPos - v_worldPos);

	vec3 result = vec3(0.0);

	for (int i = 0; i < u_lightCount && i < MAX_LIGHTS; ++i)
	{
		LightData light = u_lights[i];

		if (light.type == 0)
			result += CalcDirectionalLight(light, normal, V, albedo, specular, ambient, u_shininess);
		else if (light.type == 1)
			result += CalcPointLight(light, v_worldPos, normal, V, albedo, specular, ambient, u_shininess);
		else if (light.type == 2)
			result += CalcSpotLight(light, v_worldPos, normal, V, albedo, specular, ambient, u_shininess);
	}

	// Emissive
	if (u_hasEmissiveMap)
		result += texture(u_emissiveMap, v_texcoord).rgb;

	o_color = vec4(result, u_opacity);
	//o_color = texture(u_albedoMap, v_texcoord);
}
)";

		const char* shadowDepthVert = R"(
#version 460 core
layout(location = 0) in vec3 a_position;

layout(std430, binding = 6) readonly buffer InstanceBuffer {
	mat4 models[];
} u_instanceData;

uniform bool  u_isInstanced;
uniform mat4  u_lightVP;
uniform mat4  u_model;

void main()
{
	mat4 model = u_isInstanced ? u_instanceData.models[gl_InstanceID] : u_model;
	gl_Position = u_lightVP * model * vec4(a_position, 1.0);
}
)";

		const char* shadowDepthFrag = R"(
#version 460 core
void main() {}
)";

	// Point light shadow depth with layered cubemap rendering
	const char* pointShadowVert = R"(
#version 460 core
layout(location = 0) in vec3 a_position;

layout(std430, binding = 6) readonly buffer InstanceBuffer {
	mat4 models[];
} u_instanceData;

uniform bool  u_isInstanced;
uniform mat4  u_model;

out vec3 v_worldPos;

void main()
{
	mat4 model = u_isInstanced ? u_instanceData.models[gl_InstanceID] : u_model;
	vec4 worldPos = model * vec4(a_position, 1.0);
	v_worldPos = worldPos.xyz;
	gl_Position = worldPos;
}
)";

	const char* pointShadowGeom = R"(
#version 460 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 18) out;

in vec3 v_worldPos[];
out vec3 v_faceWorldPos;

uniform mat4 u_faceVP[6];

void main()
{
	for (int face = 0; face < 6; ++face)
	{
		gl_Layer = face;
		for (int i = 0; i < 3; ++i)
		{
			v_faceWorldPos = v_worldPos[i];
			gl_Position = u_faceVP[face] * gl_in[i].gl_Position;
			EmitVertex();
		}
		EndPrimitive();
	}
}
)";

	const char* pointShadowFrag = R"(
#version 460 core

in vec3 v_faceWorldPos;

uniform vec3  u_lightPos;
uniform float u_farPlane;

void main()
{
	float dist = length(v_faceWorldPos - u_lightPos);
	gl_FragDepth = dist / u_farPlane;
}
)";

	gpu::program::ShaderProgramPtr program;
	gpu::program::ShaderProgramPtr g_depthShader;
	gpu::program::ShaderProgramPtr g_pointDepthShader;

	gr::Mesh mesh;
	gr::Material material;

	std::unique_ptr<scene::SceneManager> g_scene;

	gr::Camera camera(glm::vec3(0.4f, 1.2f, -2.4f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

// Helper: set CameraNode transform from eye/target/up
void SetupCameraLookAt(scene::CameraNode& cam,
	const glm::vec3& eye,
	const glm::vec3& target,
	const glm::vec3& up)
{
	cam.transform.position = eye;
	glm::mat4 view = glm::lookAt(eye, target, up);
	glm::mat4 world = glm::inverse(view);
	cam.transform.rotation = glm::quat_cast(glm::mat3(world));
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
	{
		gpu::program::GraphicsProgramCreateInfo createInfo{
		.name = "BlinnPhong",
		.vertexShaderCode = blinnPhongVert,
		.fragmentShaderCode = blinnPhongFrag };
		program = gpu::program::CreateShaderProgram(createInfo);
		if (!gpu::program::IsValid(program))
		{
			core::Error("SceneExample: failed to compile BlinnPhong shader");
			return false;
		}
	}
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "ShadowDepth",
			.vertexShaderCode = shadowDepthVert,
			.fragmentShaderCode = shadowDepthFrag,
		};
		g_depthShader = gpu::program::CreateShaderProgram(info);
		if (!gpu::program::IsValid(g_depthShader))
		{
			core::Error("SceneExample: failed to compile ShadowDepth shader");
			return false;
		}
	}
	{
		gpu::program::GraphicsProgramCreateInfo info{
			.name = "PointShadowDepth",
			.vertexShaderCode = pointShadowVert,
			.fragmentShaderCode = pointShadowFrag,
			.geometryShaderCode = pointShadowGeom,
		};
		g_pointDepthShader = gpu::program::CreateShaderProgram(info);
		if (!gpu::program::IsValid(g_pointDepthShader))
		{
			core::Error("SceneExample: failed to compile PointShadowDepth shader");
			return false;
		}
	}

	mesh = gr::Mesh::CreateCube();
	material.albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");

	// --- SceneManager ---
	g_scene = std::make_unique<scene::SceneManager>();
	auto& root = *g_scene->root;

	// --- Camera ---
	auto& cam = root.AddChild<scene::CameraNode>("camera");
	SetupCameraLookAt(cam,
		glm::vec3(0.0f, 1.2f, -3.0f),  // eye
		glm::vec3(0.0f, 0.0f, 0.0f),  // target
		glm::vec3(0.0f, 1.0f, 0.0f)); // up
	cam.aspectRatio = window::GetAspectRatio();
	cam.externalCamera = &camera;

	g_scene->SetActiveCamera(cam);

	// --- Instanced cubes ---
	auto sharedMesh = std::make_shared<gr::Mesh>(gr::Mesh::CreateCube());
	auto sharedMat = std::make_shared<gr::Material>();
	sharedMat->albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");
	sharedMat->albedoColor = glm::vec3(1.0f);
	sharedMat->specularColor = glm::vec3(1.0f);
	sharedMat->ambientColor = glm::vec3(0.08f);
	sharedMat->shininess = 32.0f;

	for (int i = 0; i < 9; ++i)
	{
		auto& node = root.AddChild<scene::ModelNode>("cube_" + std::to_string(i));
		node.mesh = sharedMesh;
		node.material = sharedMat;
		float x = static_cast<float>(i % 3) * 2.5f - 2.5f;
		float z = static_cast<float>(i / 3) * 2.5f - 2.5f;
		node.transform.position = glm::vec3(x, 0.1f, z);
		node.transform.rotation = glm::angleAxis(glm::radians(25.0f * (i + 1)), glm::vec3(0, 1, 0));
	}

	// --- Sphere ---
	auto& sphere = root.AddChild<scene::ModelNode>("sphere");
	sphere.mesh = std::make_shared<gr::Mesh>(gr::Mesh::CreateSphere(32,32));
	sphere.material = std::make_shared<gr::Material>();
	sphere.material->albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");
	sphere.material->albedoColor = glm::vec3(0.8f, 0.2f, 0.8f);
	sphere.material->specularColor = glm::vec3(1.0f);
	sphere.material->ambientColor = glm::vec3(0.08f);
	sphere.material->shininess = 32.0f;

	sphere.transform.position = glm::vec3(4.0f, 1.0f, 0.0f);

	// --- Ground plane ---
	auto& plane = root.AddChild<scene::ModelNode>("ground");
	plane.mesh = std::make_shared<gr::Mesh>(gr::Mesh::CreatePlane(20.0f));
	plane.material = std::make_shared<gr::Material>();
	plane.material->albedoColor = glm::vec3(0.25f, 0.80f, 0.22f);
	plane.material->albedoMap = gpu::texture::LoadTexture2D("data/textures/uv.png");
	plane.material->specularColor = glm::vec3(0.1f);
	plane.material->ambientColor = glm::vec3(0.04f);
	plane.material->shininess = 8.0f;
	plane.transform.position = glm::vec3(0.0f, -0.5f, 0.0f);
	//plane.castShadow = false;

	// --- Directional light (shines along local -Y: (0,-1,0) by default) ---
	auto& sun = root.AddChild<scene::LightNode>("sun");
	sun.lightType = scene::LightNode::LightType::Directional;
	sun.color = glm::vec3(1.0f, 0.95f, 0.85f);
	sun.intensity = 1.2f;
	sun.castShadow = true;
	sun.shadowSettings.resolution = 4096;
	sun.shadowSettings.orthoSize = 34.0f;
	// Disable cascade splitting so the shadow pass uses a single ortho matrix
	sun.shadowSettings.cascadeDistance[0] = -1.0f;
	sun.shadowSettings.cascadeDistance[1] = -1.0f;
	sun.shadowSettings.cascadeDistance[2] = -1.0f;
	sun.shadowSettings.cascadeDistance[3] = -1.0f;
	// Rotate so light comes from upper-right (local (0,-1,0) → world normalize(1,-1,0))
	sun.transform.rotation = glm::angleAxis(glm::radians(45.0f), glm::vec3(0, 0, 1));

	// --- Point light (warm, with shadow) ---
	auto& pointLight = root.AddChild<scene::LightNode>("point_light");
	pointLight.lightType = scene::LightNode::LightType::Point;
	pointLight.color = glm::vec3(0.4f, 0.2f, 0.3f);
	pointLight.intensity = 6.0f;
	pointLight.radius = 8.0f;
	pointLight.attenuation = glm::vec3(1.0f, 0.09f, 0.032f);
	pointLight.castShadow = true;
	pointLight.shadowSettings.resolution = 256;
	pointLight.transform.position = glm::vec3(2.0f, 2.5f, 1.0f);

	// --- Stress test: 13 more lights to reach MAX_LIGHTS (16) ---
	auto addPoint = [&](std::string name, glm::vec3 pos, glm::vec3 color,
		float intensity, float radius, bool shadow)
		{
			auto& l = root.AddChild<scene::LightNode>(name);
			l.lightType = scene::LightNode::LightType::Point;
			l.transform.position = pos;
			l.color = color;
			l.intensity = intensity;
			l.radius = radius;
			l.attenuation = glm::vec3(1.0f, 0.09f, 0.032f);
			l.castShadow = shadow;
			l.shadowSettings.resolution = 256;
		};

	//addPoint("p_r", glm::vec3(-2.5f, 1.5f, -2.5f), glm::vec3(1, 0, 0), 3.0f, 6.0f, true);
	//addPoint("p_g", glm::vec3(2.5f, 1.5f, -2.5f), glm::vec3(0, 1, 0), 3.0f, 6.0f, true);
	//addPoint("p_b", glm::vec3(0.0f, 1.5f, 2.5f), glm::vec3(0, 0, 1), 3.0f, 6.0f, true);
	//addPoint("p_y", glm::vec3(-2.0f, 0.8f, 1.5f), glm::vec3(1, 1, 0), 2.5f, 5.0f, true);
	//addPoint("p_c", glm::vec3(3.0f, 0.8f, 0.5f), glm::vec3(0, 1, 1), 2.5f, 5.0f, true);
	//addPoint("p_m", glm::vec3(0.5f, 0.8f, -3.0f), glm::vec3(1, 0, 1), 2.5f, 5.0f, true);
	//addPoint("p_w", glm::vec3(-3.5f, 2.0f, 3.5f), glm::vec3(1, 1, 1), 4.0f, 7.0f, true);
	//addPoint("p_o", glm::vec3(3.5f, 2.0f, 3.0f), glm::vec3(1, 0.5, 0), 3.5f, 6.0f, true);

	//// Replace spot lights with point lights — all 16 use shadow maps
	//addPoint("s1", glm::vec3(-3.0f, 3.0f, 0.0f), glm::vec3(0.9f, 0.2f, 0.1f), 5.0f, 7.0f, true);
	//addPoint("s2", glm::vec3(3.5f, 3.0f, -3.0f), glm::vec3(0.1f, 0.3f, 0.9f), 5.0f, 7.0f, true);
	//addPoint("s3", glm::vec3(-1.0f, 3.5f, 3.5f), glm::vec3(0.2f, 0.8f, 0.2f), 4.0f, 6.0f, true);
	//addPoint("s4", glm::vec3(4.0f, 2.5f, 2.0f), glm::vec3(0.8f, 0.8f, 0.1f), 4.5f, 6.5f, true);
	//addPoint("s5", glm::vec3(-4.0f, 2.0f, -3.0f), glm::vec3(0.9f, 0.2f, 0.7f), 3.5f, 6.0f, true);

	//// 16-й: dim warm fill from below
	//addPoint("p_fill", glm::vec3(0.0f, -0.4f, 0.0f), glm::vec3(0.6f, 0.3f, 0.1f), 1.5f, 4.0f, true);

	g_scene->enableShadows = true;
	g_scene->enableInstancing = true;

	return true;
}
//=============================================================================
void GameClose()
{
	g_pointDepthShader.reset();
	g_depthShader.reset();
	g_scene.reset();
	g_mouseLook.Reset();
	program.reset();
	mesh.Close();
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

	// Update camera aspect ratio
	if (g_scene->activeCamera)
		g_scene->activeCamera->aspectRatio = window::GetAspectRatio();

	// Animate: spin cube slowly
	//auto* cube = g_scene->root->FindChild("cube");
	/*if (cube)
		cube->transform.rotation *= glm::angleAxis(app::GetDeltaTime() * 5.5f, glm::vec3(0, 1, 0));*/

	// Traverse scene graph → update world matrices, collect lights
	g_scene->Update();

}
//=============================================================================
void GameFixedUpdate()
{
}
//=============================================================================
void GameRender()
{
	math::Frustum frustum;
	if (g_scene->activeCamera)
		frustum = g_scene->activeCamera->ExtractFrustum();

	// 1. Shadow passes for all shadow-casting lights
	if (g_scene->enableShadows)
	{
		for (auto* light : g_scene->lights)
		{
			if (!light->castShadow) continue;
			auto shadowQueue = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Shadow);
			if (light->lightType == scene::LightNode::LightType::Point)
				g_scene->RenderShadowPass(shadowQueue, *light, g_depthShader, g_pointDepthShader);
			else
				g_scene->RenderShadowPass(shadowQueue, *light, g_depthShader);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
	}

	// 2. Clear main framebuffer
	glClearColor(0.12f, 0.32f, 0.88f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// 3. Viewport for the main framebuffer
	gpu::Viewport vp;
	vp.drawRect.offset = { 0, 0 };
	vp.drawRect.extent = { window::GetWidth(), window::GetHeight() };
	gpu::cmd::SetViewport(vp);

	// 4. Build render queue with frustum culling from active camera
	g_scene->enableFrustumCulling = true;
	auto queue = g_scene->BuildRenderQueue(frustum, scene::RenderPassType::Opaque);
	
	// 5. Render opaque objects (shadow map is bound inside renderOpaquePass)
	g_scene->RenderOpaquePass(queue, program);
	g_scene->RenderTransparentPass(queue, program);
}
//=============================================================================
void GameRenderUI()
{
	ImGui::Begin("Hello, world!");
	ImGui::Text("Scene");
	ImGui::Text("Draw calls: %u", g_scene->lastFrameStats.drawCalls);
	ImGui::Text("Instanced:  %u", g_scene->lastFrameStats.instancedBatches);
	ImGui::Text("Culled:     %u", g_scene->lastFrameStats.culledObjects);
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