#include "stdafx.h"
#include "sc_sceneManager.h"
#include "gr_mesh.h"
#include "gr_material.h"
#include "gpu_cmd.h"
//=============================================================================
// Helper: pair of raw pointers used as key for auto-instancing
// Must match GLSL std140 layout in blinnPhongFrag
struct alignas(16) LightDataGPU
{
	glm::vec4 positionOrDirection; // 0
	glm::vec3 color;               // 16
	float     intensity;           // 28
	glm::vec3 attenuation;         // 32
	float     radius;              // 44
	glm::vec3 spotDirection;       // 48
	float     innerCutoff;         // 60
	float     outerCutoff;         // 64
	int32_t   type;                // 68
	int32_t   castShadow;          // 72
	float     shadowBias;          // 76
	// implicit pad to 80 (16-aligned)
	glm::mat4 lightSpaceMatrix;    // 80
};
static_assert(sizeof(LightDataGPU) == 144, "LightDataGPU std140 size mismatch");

struct alignas(16) LightBlockUBO
{
	int32_t      lightCount;
	uint8_t      _pad[12];
	LightDataGPU lights[16];
};
static_assert(sizeof(LightBlockUBO) == 4 + 12 + 144 * 16, "LightBlockUBO std140 size mismatch");

struct MeshMaterialPair
{
	const gr::Mesh* mesh;
	const gr::Material* material;
	bool operator==(const MeshMaterialPair&) const = default;
};
//=============================================================================
struct PairHash
{
	size_t operator()(const MeshMaterialPair& p) const noexcept
	{
		return std::hash<const void*>{}(p.mesh) ^ (std::hash<const void*>{}(p.material) << 1);
	}
};
//=============================================================================
// Auto-instancing group: a collection of world transforms that share the same mesh+material
struct InstanceGroup
{
	std::vector<glm::mat4> transforms;
	scene::ModelNode* firstNode = nullptr;
	float      distance = 0.0f;
};
//=============================================================================
scene::SceneManager::SceneManager()
{
	root = std::make_unique<SceneNode>("root");

	m_wireframeState.polygonMode = gpu::RasterizationMode::Line;
	m_wireframeState.cullMode = gpu::CullFace::None;
	m_fillState.polygonMode = gpu::RasterizationMode::Fill;
	m_fillState.cullMode = gpu::CullFace::Back;
	m_fillState.frontFace = gpu::FrontFace::CounterClockWise;

	gpu::texture::SamplerState ss;
	ss.minFilter = gpu::Filter::Linear;
	ss.magFilter = gpu::Filter::Linear;
	ss.addressModeU = gpu::AddressMode::ClampToEdge;
	ss.addressModeV = gpu::AddressMode::ClampToEdge;
	ss.compareEnable = false;
	m_shadowSampler = gpu::texture::CreateSampler(ss);

	m_instanceCapacity = INITIAL_INSTANCE_CAPACITY;
	m_instanceSSBO = gpu::buffer::CreateBuffer(
		m_instanceCapacity * sizeof(glm::mat4),
		gpu::BufferStorageFlag::DynamicStorage,
		"instance_ssbo");

	m_lightUBO = gpu::buffer::CreateBuffer(
		sizeof(LightBlockUBO),
		gpu::BufferStorageFlag::DynamicStorage,
		"light_ubo");
}
//=============================================================================
void scene::SceneManager::Update()
{
	lights.clear();
	reflectionProbes.clear();

	if (!root) return;

	// Traverse graph, updating world matrices and collecting references
	root->Traverse([this](SceneNode& node, const glm::mat4& world)
		{
			switch (node.type)
			{
			case NodeType::Light:
				lights.push_back(static_cast<LightNode*>(&node));
				break;
			case NodeType::ReflectionProbe:
				reflectionProbes.push_back(static_cast<ReflectionProbeNode*>(&node));
				break;
			default:
				break;
			}
		});
}
//=============================================================================
gr::RenderQueue scene::SceneManager::BuildRenderQueue(const math::Frustum& frustum, RenderPassType passType)
{
	gr::RenderQueue queue;
	lastFrameStats = {};

	if (!root) return queue;

	glm::vec3 cameraPos = GetActiveCameraPosition();

	// Map for auto-instancing: (mesh*, material*) -> InstanceGroup
	std::unordered_map<MeshMaterialPair, InstanceGroup, PairHash> instanceGroups;

	root->Traverse([&](SceneNode& node, const glm::mat4& world)
		{
			if (!node.visible) return;
			if (node.type != NodeType::Model) return;

			auto* modelNode = static_cast<ModelNode*>(&node);
			if (!modelNode->mesh || !modelNode->material) return;

			// Filter by reflection pass exclusion
			if (passType == RenderPassType::Reflection && modelNode->excludeFromReflections)
				return;

			// Filter by shadow caster flag
			if (passType == RenderPassType::Shadow && !modelNode->castShadow)
				return;

			// Frustum culling (skip for shadow pass where we want all casters)
			if (enableFrustumCulling && passType != RenderPassType::Shadow)
			{
				if (!math::TestAABB(frustum, modelNode->mesh->aabb, world))
				{
					++lastFrameStats.culledObjects;
					return;
				}
			}

			float dist = glm::distance(cameraPos, glm::vec3(world[3]));

			// Check for manual instancing
			if (!modelNode->instanceTransforms.empty())
			{
				gr::RenderItem item;
				item.node = modelNode;
				item.worldTransform = world;
				item.distanceToCamera = dist;
				item.materialId = reinterpret_cast<uintptr_t>(modelNode->material.get());
				item.isInstanced = true;

				bool isTransparent = modelNode->material->IsTransparent();
				queue.Submit(item, isTransparent);
				++lastFrameStats.instancedBatches;
				return;
			}

			// Auto-instancing: group by shared mesh+material pointers
			if (enableInstancing)
			{
				MeshMaterialPair key{ modelNode->mesh.get(), modelNode->material.get() };
				auto& group = instanceGroups[key];
				group.transforms.push_back(world);
				group.distance += dist;
				if (!group.firstNode)
					group.firstNode = modelNode;
			}
			else
			{
				// No instancing: submit individually
				gr::RenderItem item;
				item.node = modelNode;
				item.worldTransform = world;
				item.distanceToCamera = dist;
				item.materialId = reinterpret_cast<uintptr_t>(modelNode->material.get());
				item.isInstanced = false;

				bool isTransparent = modelNode->material->IsTransparent();
				queue.Submit(item, isTransparent);
				++lastFrameStats.drawCalls;
			}
		});

	// Emit auto-instanced groups
	for (auto& [key, group] : instanceGroups)
	{
		if (!group.firstNode) continue;

		// If only one transform, submit as non-instanced
		if (group.transforms.size() == 1)
		{
			gr::RenderItem item;
			item.node = group.firstNode;
			item.worldTransform = group.transforms[0];
			item.distanceToCamera = group.distance;
			item.materialId = reinterpret_cast<uintptr_t>(group.firstNode->material.get());
			item.isInstanced = false;

			bool isTransparent = group.firstNode->material->IsTransparent();
			queue.Submit(item, isTransparent);
			++lastFrameStats.drawCalls;
		}
		else
		{
			// Use the first node to represent the group, store transforms in its instanceTransforms
			group.firstNode->instanceTransforms = std::move(group.transforms);

			// Average distance
			float avgDist = group.distance / static_cast<float>(group.firstNode->instanceTransforms.size());

			gr::RenderItem item;
			item.node = group.firstNode;
			item.worldTransform = group.firstNode->instanceTransforms[0];
			item.distanceToCamera = avgDist;
			item.materialId = reinterpret_cast<uintptr_t>(group.firstNode->material.get());
			item.isInstanced = true;

			bool isTransparent = group.firstNode->material->IsTransparent();
			queue.Submit(item, isTransparent);
			++lastFrameStats.instancedBatches;
		}
	}

	return queue;
}
//=============================================================================
void scene::SceneManager::RenderShadowPass(gr::RenderQueue& queue, LightNode& light, const gpu::program::ShaderProgramPtr& depthShader)
{
	if (!enableShadows || !depthShader) return;

	gr::ShadowMap& sm = shadowMaps.GetOrCreate(&light);

	// Compute light VP matrix
	glm::mat4 lightProj, lightView;

	glm::vec3 worldDir;
	if (light.lightType == LightNode::LightType::Directional)
	{
		worldDir = glm::normalize(glm::mat3(light.cachedWorldMatrix) * light.direction);
		glm::vec3 camPos = GetActiveCameraPosition();
		glm::vec3 up = std::abs(worldDir.y) < 0.99f
			? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
		
		glm::vec3 eye = camPos - worldDir * 200.0f;
		lightView = glm::lookAt(eye, camPos, up);
		float s = light.shadowSettings.orthoSize;

		// Compute tight z bounds from all render items
		float minZ = std::numeric_limits<float>::max();
		float maxZ = -std::numeric_limits<float>::max();
		auto computeZBounds = [&](const std::vector<gr::RenderItem>& items)
			{
				for (auto& item : items)
				{
					if (!item.node || !item.node->mesh) continue;
					math::AABB worldAABB = item.node->mesh->aabb.Transform(item.worldTransform);
					const glm::vec3 corners[8] = {
						{worldAABB.min.x, worldAABB.min.y, worldAABB.min.z},
						{worldAABB.min.x, worldAABB.min.y, worldAABB.max.z},
						{worldAABB.min.x, worldAABB.max.y, worldAABB.min.z},
						{worldAABB.min.x, worldAABB.max.y, worldAABB.max.z},
						{worldAABB.max.x, worldAABB.min.y, worldAABB.min.z},
						{worldAABB.max.x, worldAABB.min.y, worldAABB.max.z},
						{worldAABB.max.x, worldAABB.max.y, worldAABB.min.z},
						{worldAABB.max.x, worldAABB.max.y, worldAABB.max.z},
					};
					for (auto& c : corners)
					{
						float z = -glm::dot(c - eye, worldDir);
						minZ = std::min(minZ, z);
						maxZ = std::max(maxZ, z);
					}
				}
			};
		computeZBounds(queue.opaqueItems);
		computeZBounds(queue.transparentItems);

		float margin = 20.0f;
		if (minZ == std::numeric_limits<float>::max())
		{
			minZ = -500.0f;
			maxZ = 500.0f;
		}
		else
		{
			minZ -= margin;
			maxZ += margin;
		}

		// nearDist is the closer clip distance (farthest scene point from light)
		// farDist is the farther clip distance (closest scene point to light)
		float nearDist = -maxZ;
		float farDist = -minZ;
		lightProj = glm::ortho(-s, s, -s, s, nearDist, farDist);

		sm.lightSpaceMatrix = lightProj * lightView;

		// CSM cascades
		for (int c = 0; c < 4; ++c)
		{
			if (light.shadowSettings.cascadeDistance[c] <= 0.0f)
			{
				sm.cascadeMatrices[c] = sm.lightSpaceMatrix;
				continue;
			}
			// Simplified cascade: tighten orthographic bounds based on cascade distance
			float d = light.shadowSettings.cascadeDistance[c];
			float cs = d * 0.5f;
			glm::mat4 cascadeProj = glm::ortho(-cs, cs, -cs, cs, -500.0f, 500.0f);
			sm.cascadeMatrices[c] = cascadeProj * lightView;
		}
	}
	else if (light.lightType == LightNode::LightType::Spot)
	{
		glm::vec3 worldPos = glm::vec3(light.cachedWorldMatrix[3]);
		worldDir = glm::normalize(glm::mat3(light.cachedWorldMatrix) * light.direction);
		glm::vec3 up = std::abs(worldDir.y) < 0.99f
			? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
		lightView = glm::lookAt(worldPos, worldPos + worldDir, up);
		lightProj = glm::perspective(light.outerAngle * 2.0f, 1.0f, 0.1f, light.radius);
		sm.lightSpaceMatrix = lightProj * lightView;
	}

	// Bind depth FBO
	gpu::cmd::BindFramebuffer(sm.framebuffer);

	// Set shadow-specific state
	gpu::DepthState depthState;
	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;
	depthState.depthCompareOp = gpu::CompareOp::Less;
	gpu::cmd::SetState(depthState);

	gpu::RasterizationState rs;
	rs.polygonMode = gpu::RasterizationMode::Fill;
	rs.cullMode = gpu::CullFace::Back;
	rs.depthBiasEnable = true;
	rs.depthBiasConstantFactor = light.shadowSettings.bias;
	rs.depthBiasSlopeFactor = light.shadowSettings.normalBias;
	gpu::cmd::SetState(rs);

	gpu::cmd::BindShaderProgram(depthShader);

	// Set light VP uniform
	int locLightVP = gpu::program::GetUniformLocation(depthShader, "u_lightVP");
	int locModel = gpu::program::GetUniformLocation(depthShader, "u_model");
	int locInst = gpu::program::GetUniformLocation(depthShader, "u_isInstanced");

	// Lambda to draw a single render item (handles instancing via SSBO)
	auto drawShadowItem = [&](const gr::RenderItem& item)
		{
			if (!item.node || !item.node->mesh) return;
			const auto& mesh = *item.node->mesh;
			mesh.Bind();

			if (item.isInstanced)
			{
				const auto& transforms = item.node->instanceTransforms;
				if (transforms.empty()) return;

				uint32_t count = static_cast<uint32_t>(transforms.size());
				if (count > m_instanceCapacity)
				{
					m_instanceCapacity = (std::max)(count, m_instanceCapacity * 2u);
					m_instanceSSBO = gpu::buffer::CreateBuffer(
						m_instanceCapacity * sizeof(glm::mat4),
						gpu::BufferStorageFlag::DynamicStorage,
						"instance_ssbo");
				}

				gpu::buffer::UpdateData(m_instanceSSBO,
					reinterpret_cast<const void*>(transforms.data()),
					count * sizeof(glm::mat4));
				gpu::cmd::BindStorageBuffer(INSTANCE_SSBO_BINDING, m_instanceSSBO);
				gpu::program::SetUniform(depthShader, locInst, true);

				mesh.DrawInstanced(count);
				++lastFrameStats.instancedBatches;
				++lastFrameStats.drawCalls;
			}
			else
			{
				gpu::program::SetUniform(depthShader, locModel, item.worldTransform);
				gpu::program::SetUniform(depthShader, locInst, false);
				mesh.Draw();
				++lastFrameStats.drawCalls;
			}
		};

	if (light.lightType == LightNode::LightType::Directional)
	{
		// Render each cascade
		int cascadeCount = 0;
		for (int c = 0; c < 4; ++c)
			if (light.shadowSettings.cascadeDistance[c] > 0.0f) ++cascadeCount;
		if (cascadeCount == 0) cascadeCount = 1;

		for (int c = 0; c < cascadeCount; ++c)
		{
			gpu::program::SetUniform(depthShader, locLightVP, sm.cascadeMatrices[c]);

			// Set viewport for this cascade (could use different regions of an atlas)
			int res = sm.resolution;
			gpu::Viewport vp;
			vp.drawRect.offset = { 0, 0 };
			vp.drawRect.extent = { static_cast<uint32_t>(res), static_cast<uint32_t>(res) };
			gpu::cmd::SetViewport(vp);

			for (auto& item : queue.opaqueItems) drawShadowItem(item);
			for (auto& item : queue.transparentItems) drawShadowItem(item);
		}
	}
	else if (light.lightType == LightNode::LightType::Spot)
	{
		gpu::program::SetUniform(depthShader, locLightVP, sm.lightSpaceMatrix);
		int res = sm.resolution;
		gpu::Viewport vp;
		vp.drawRect.offset = { 0, 0 };
		vp.drawRect.extent = { static_cast<uint32_t>(res), static_cast<uint32_t>(res) };
		gpu::cmd::SetViewport(vp);

		for (auto& item : queue.opaqueItems) drawShadowItem(item);
		for (auto& item : queue.transparentItems) drawShadowItem(item);
	}
	// Point light cubemap shadow would render 6 times with different face matrices
}
//=============================================================================
void scene::SceneManager::RenderOpaquePass(gr::RenderQueue& queue, const gpu::program::ShaderProgramPtr& blinnPhongShader)
{
	if (!blinnPhongShader) return;

	queue.Sort();

	gpu::DepthState depthState;
	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = true;
	depthState.depthCompareOp = gpu::CompareOp::Less;
	gpu::cmd::SetState(depthState);

	// Wireframe mode
	if (wireframeMode)
		gpu::cmd::SetState(m_wireframeState);
	else
		gpu::cmd::SetState(m_fillState);

	gpu::cmd::BindShaderProgram(blinnPhongShader);

	// Upload camera and lights
	UploadCamera(blinnPhongShader);
	UploadLights(blinnPhongShader);
	UploadShadowMap(blinnPhongShader);

	for (auto& item : queue.opaqueItems)
	{
		if (!item.node || !item.node->mesh) continue;
		drawRenderItem(item, blinnPhongShader, true);
	}

	// Restore fill state if wireframe was active
	if (wireframeMode)
		gpu::cmd::SetState(m_fillState);
}
//=============================================================================
void scene::SceneManager::RenderTransparentPass(gr::RenderQueue& queue, const gpu::program::ShaderProgramPtr& blinnPhongShader)
{
	if (!blinnPhongShader || queue.transparentItems.empty()) return;

	queue.Sort();

	gpu::DepthState depthState;
	depthState.depthTestEnable = true;
	depthState.depthWriteEnable = false; // Don't write depth for transparency
	depthState.depthCompareOp = gpu::CompareOp::Less;
	gpu::cmd::SetState(depthState);

	gpu::ColorBlendAttachmentState blend;
	blend.blendEnable = true;
	blend.srcColorBlendFactor = gpu::BlendFactor::SrcAlpha;
	blend.dstColorBlendFactor = gpu::BlendFactor::OneMinusSrcAlpha;
	blend.srcAlphaBlendFactor = gpu::BlendFactor::One;
	blend.dstAlphaBlendFactor = gpu::BlendFactor::OneMinusSrcAlpha;
	gpu::ColorBlendState colorBlend;
	colorBlend.attachments.push_back(blend);
	gpu::cmd::SetState(colorBlend);

	gpu::cmd::SetState(m_fillState);
	gpu::cmd::BindShaderProgram(blinnPhongShader);

	UploadCamera(blinnPhongShader);
	UploadLights(blinnPhongShader);

	for (auto& item : queue.transparentItems)
	{
		if (!item.node || !item.node->mesh) continue;
		drawRenderItem(item, blinnPhongShader, true);
	}

	// Reset blend state
	gpu::ColorBlendState noBlend;
	gpu::cmd::SetState(noBlend);
}
//=============================================================================
void scene::SceneManager::RenderReflectionPass(ReflectionProbeNode& probe, const gpu::program::ShaderProgramPtr& shader)
{
	if (!shader || !probe.isDirty) return;

	// Render scene 6 times into cubemap faces
	auto captureMatrices = probe.GetCaptureMatrices();

	// For each face:
	// 1. Set viewport to cubemap face size
	// 2. Bind face as render target
	// 3. Render scene with face's VP matrix
	// 4. Set probe.isDirty = false

	// Note: This requires a cubemap framebuffer which is set up by the rendering system.
	// The SceneManager simply provides the matrices and leaves the FBO management to the caller.

	probe.isDirty = false;
}
//=============================================================================
void scene::SceneManager::UploadLights(const gpu::program::ShaderProgramPtr& shader)
{
	if (!m_lightUBO) return;

	LightBlockUBO block{};
	block.lightCount = static_cast<int32_t>((std::min)(lights.size(), (size_t)MAX_LIGHTS));

	for (size_t i = 0; i < lights.size() && i < MAX_LIGHTS; ++i)
	{
		auto* light = lights[i];
		auto& data = block.lights[i];

		data.color = light->color;
		data.intensity = light->intensity;
		data.shadowBias = light->shadowSettings.bias;

		switch (light->lightType)
		{
		case LightNode::LightType::Directional:
		{
			// Direction in world space: transform local direction by world matrix
			glm::vec3 worldDir = glm::normalize(glm::mat3(light->cachedWorldMatrix) * light->direction);
			data.positionOrDirection = glm::vec4(worldDir, 0.0f);
			data.type = 0;
			data.spotDirection = worldDir;
			data.innerCutoff = 0.0f;
			data.outerCutoff = 0.0f;
			data.castShadow = light->castShadow ? 1 : 0;

			// Use precomputed light space matrix from shadow pass if available
			auto* sm = shadowMaps.Get(light);
			if (sm)
				data.lightSpaceMatrix = sm->lightSpaceMatrix;
			else
			{
				glm::vec3 camPos = GetActiveCameraPosition();
				glm::vec3 up = std::abs(worldDir.y) < 0.99f
					? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
				glm::mat4 lightView = glm::lookAt(camPos - worldDir * 200.0f, camPos, up);
				float s = light->shadowSettings.orthoSize;
				glm::mat4 lightProj = glm::ortho(-s, s, -s, s, -500.0f, 500.0f);
				data.lightSpaceMatrix = lightProj * lightView;
			}
			break;
		}
		case LightNode::LightType::Point:
		{
			glm::vec3 worldPos = glm::vec3(light->cachedWorldMatrix[3]);
			data.positionOrDirection = glm::vec4(worldPos, 1.0f);
			data.type = 1;
			data.radius = light->radius;
			data.attenuation = light->attenuation;
			break;
		}
		case LightNode::LightType::Spot:
		{
			glm::vec3 worldPos = glm::vec3(light->cachedWorldMatrix[3]);
			glm::vec3 worldDir = glm::normalize(glm::mat3(light->cachedWorldMatrix) * light->direction);
			data.positionOrDirection = glm::vec4(worldPos, 2.0f);
			data.type = 2;
			data.radius = light->radius;
			data.attenuation = light->attenuation;
			data.spotDirection = worldDir;
			data.innerCutoff = cos(light->innerAngle);
			data.outerCutoff = cos(light->outerAngle);
			data.castShadow = light->castShadow ? 1 : 0;

			// Light VP for spot shadow
			glm::mat4 lightView = glm::lookAt(worldPos, worldPos + worldDir,
				std::abs(worldDir.y) < 0.99f ? glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0));
			glm::mat4 lightProj = glm::perspective(light->outerAngle * 2.0f, 1.0f, 0.1f, light->radius);
			data.lightSpaceMatrix = lightProj * lightView;
			break;
		}
		}
	}

	gpu::buffer::UpdateData(m_lightUBO, static_cast<const void*>(&block), sizeof(LightBlockUBO));
	gpu::cmd::BindUniformBuffer(LIGHT_UBO_BINDING, m_lightUBO, 0, sizeof(LightBlockUBO));
}
//=============================================================================
void scene::SceneManager::UploadCamera(const gpu::program::ShaderProgramPtr& shader)
{
	if (!activeCamera) return;

	int locView = gpu::program::GetUniformLocation(shader, "u_view");
	int locProj = gpu::program::GetUniformLocation(shader, "u_projection");
	int locCamPos = gpu::program::GetUniformLocation(shader, "u_cameraPos");

	gpu::program::SetUniform(shader, locView, activeCamera->GetViewMatrix());
	gpu::program::SetUniform(shader, locProj, activeCamera->GetProjectionMatrix());
	gpu::program::SetUniform(shader, locCamPos, GetActiveCameraPosition());
}
//=============================================================================
void scene::SceneManager::UploadModel(const gpu::program::ShaderProgramPtr& shader, const glm::mat4& model, const glm::mat3& normalMatrix)
{
	int locModel = gpu::program::GetUniformLocation(shader, "u_model");
	int locNormal = gpu::program::GetUniformLocation(shader, "u_normalMatrix");
	gpu::program::SetUniform(shader, locModel, model);
	gpu::program::SetUniform(shader, locNormal, normalMatrix);
}
//=============================================================================
// Upload shadow map texture + metadata
void scene::SceneManager::UploadShadowMap(const gpu::program::ShaderProgramPtr& shader)
{
	// Find the first shadow-casting light
	LightNode* shadowLight = nullptr;
	for (auto* light : lights)
	{
		if (light->castShadow)
		{
			shadowLight = light;
			break;
		}
	}
	if (!shadowLight) return;

	auto& sm = shadowMaps.GetOrCreate(shadowLight);
	if (!sm.depthTexture) return;

	gpu::cmd::BindSampledImage(SHADOW_TEX_UNIT, sm.depthTexture, m_shadowSampler);

	auto setInt = [&](const char* name, int val) {
		int loc = gpu::program::GetUniformLocation(shader, name);
		gpu::program::SetUniform(shader, loc, val);
		};
	auto setFloat = [&](const char* name, float val) {
		int loc = gpu::program::GetUniformLocation(shader, name);
		gpu::program::SetUniform(shader, loc, val);
		};

	setInt("u_shadowMap", static_cast<int>(SHADOW_TEX_UNIT));
	setFloat("u_shadowMapSize", static_cast<float>(sm.resolution));
}
//=============================================================================
glm::vec3 scene::SceneManager::GetActiveCameraPosition() const
{
	if (!activeCamera) return glm::vec3(0.0f);
	return activeCamera->GetPosition();
}
//=============================================================================
void scene::SceneManager::drawRenderItem(const gr::RenderItem& item, const gpu::program::ShaderProgramPtr& shader, bool receiveShadowUniform)
{
	if (!item.node || !item.node->mesh || !item.node->material) return;

	const auto& mesh = *item.node->mesh;
	auto& material = *item.node->material;

	// Upload receiveShadow uniform
	if (receiveShadowUniform)
	{
		int loc = gpu::program::GetUniformLocation(shader, "u_receiveShadow");
		gpu::program::SetUniform(shader, loc, item.node->receiveShadow);
	}

	// Bind material
	material.Bind(shader);

	// Bind mesh and draw
	mesh.Bind();

	if (item.isInstanced)
	{
		// Use node's instance transforms
		const auto& transforms = item.node->instanceTransforms;
		if (transforms.empty()) return;
		uint32_t count = static_cast<uint32_t>(transforms.size());

		// Grow SSBO if needed
		if (count > m_instanceCapacity)
		{
			m_instanceCapacity = (std::max)(count, m_instanceCapacity * 2u);
			m_instanceSSBO = gpu::buffer::CreateBuffer(
				m_instanceCapacity * sizeof(glm::mat4),
				gpu::BufferStorageFlag::DynamicStorage,
				"instance_ssbo");
		}

		// Upload all instance matrices to SSBO
		gpu::buffer::UpdateData(m_instanceSSBO,
			reinterpret_cast<const void*>(transforms.data()),
			count * sizeof(glm::mat4));

		// Bind SSBO and set instancing flag
		gpu::cmd::BindStorageBuffer(INSTANCE_SSBO_BINDING, m_instanceSSBO);

		int locInst = gpu::program::GetUniformLocation(shader, "u_isInstanced");
		gpu::program::SetUniform(shader, locInst, true);

		// Single draw call for all instances
		mesh.DrawInstanced(count);
		++lastFrameStats.instancedBatches;
		++lastFrameStats.drawCalls;
	}
	else
	{
		// Upload model matrix for single instance
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(item.worldTransform)));
		UploadModel(shader, item.worldTransform, normalMatrix);

		int locInst = gpu::program::GetUniformLocation(shader, "u_isInstanced");
		gpu::program::SetUniform(shader, locInst, false);

		mesh.Draw();
		++lastFrameStats.drawCalls;
	}
}
//=============================================================================