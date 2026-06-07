// Map shader for rendering tile geometry with instancing support
constexpr auto g_mapShaderVert = R"(
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

out vec3 v_worldPos;
out vec3 v_worldNormal;
out vec2 v_texcoord;

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

constexpr auto g_mapShaderFrag = R"(
#version 460 core
layout(location = 0) in vec3 v_worldPos;
layout(location = 1) in vec3 v_worldNormal;
layout(location = 2) in vec2 v_texcoord;

layout(location = 0) out vec4 o_color;

uniform vec3  u_albedoColor;
uniform vec3  u_ambientColor;
uniform bool  u_hasAlbedoMap;
layout(binding = 0) uniform sampler2D u_albedoMap;

void main()
{
	vec3 albedo = u_albedoColor;
	if (u_hasAlbedoMap) albedo *= texture(u_albedoMap, v_texcoord).rgb;
	vec3 light = u_ambientColor + max(dot(v_worldNormal, normalize(vec3(0.5, 1.0, 0.3))), 0.0) * 0.8;
	o_color = vec4(albedo * light, 1.0);
}
)";

// Shadow map shader variants
constexpr auto g_shadowDepthVert = R"(
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

constexpr auto g_shadowDepthFrag = R"(
#version 460 core
void main() {}
)";

constexpr auto g_pointShadowDepthVert = R"(
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

constexpr auto g_pointShadowDepthGeom = R"(
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

constexpr auto g_pointShadowDepthFrag = R"(
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

// ===== Blinn-Phong main shader (full PBR with shadows) =====
constexpr auto g_blinnPhongVert = R"(
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

constexpr auto g_blinnPhongFrag = R"(
#version 460 core

#define MAX_LIGHTS 16

struct LightData {
	vec4  positionOrDirection;
	vec3  color;
	float intensity;
	vec3  attenuation;
	float radius;
	vec3  spotDirection;
	float innerCutoff;
	float outerCutoff;
	int   type;
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

uniform vec3  u_albedoColor;
uniform vec3  u_specularColor;
uniform vec3  u_ambientColor;
uniform float u_shininess;
uniform float u_opacity;

uniform bool       u_hasAlbedoMap;
uniform bool       u_hasNormalMap;
uniform bool       u_hasSpecularMap;
uniform bool       u_hasEmissiveMap;
layout(binding = 0) uniform sampler2D u_albedoMap;
layout(binding = 1) uniform sampler2D u_normalMap;
layout(binding = 2) uniform sampler2D u_specularMap;
layout(binding = 3) uniform sampler2D u_emissiveMap;

uniform bool       u_receiveShadow;
uniform sampler2D  u_shadowMap;
uniform float      u_shadowMapSize;

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
		for (int y = -1; y <= 1; ++y)
		{
			float pcfDepth = texture(u_shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
			shadow += currentDepth < pcfDepth ? 1.0 : 0.0;
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
	vec3  L    = light.positionOrDirection.xyz - fragPos;
	float dist = length(L);
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
	vec3  L    = light.positionOrDirection.xyz - fragPos;
	float dist = length(L);
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
	vec3 albedo   = u_albedoColor;
	vec3 specular = u_specularColor;
	vec3 ambient  = u_ambientColor;
	vec3 normal   = normalize(v_worldNormal);

	if (u_hasAlbedoMap)   albedo   *= texture(u_albedoMap,   v_texcoord).rgb;
	if (u_hasSpecularMap) specular *= texture(u_specularMap, v_texcoord).rgb;
	if (u_hasNormalMap)
	{
		vec3 n = texture(u_normalMap, v_texcoord).xyz * 2.0 - 1.0;
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

	if (u_hasEmissiveMap)
		result += texture(u_emissiveMap, v_texcoord).rgb;

	o_color = vec4(result, u_opacity);
}
)";