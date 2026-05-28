#pragma once

#include "sc_node.h"
#include "sc_cameraNode.h"
#include "sc_lightNode.h"
#include "sc_modelNode.h"
#include "gr_renderQueue.h"
#include "gpu_pipeline.h"
#include "gpu_program.h"
#include "sc_reflectionNode.h"
#include "gr_shadowMapManager.h"

constexpr int MAX_LIGHTS = 16;

namespace scene
{
	enum class RenderPassType : uint8_t
	{
		Shadow,
		Opaque,
		Transparent,
		Reflection,
		PostProcess
	};

	struct RenderStats final
	{
		uint32_t drawCalls = 0;
		uint32_t culledObjects = 0;
		uint32_t instancedBatches = 0;
	};

	class SceneManager final
	{
	public:
		SceneManager();

		// Non-copyable, movable
		SceneManager(const SceneManager&) = delete;
		SceneManager& operator=(const SceneManager&) = delete;
		SceneManager(SceneManager&&) noexcept = default;
		SceneManager& operator=(SceneManager&&) noexcept = default;

		void SetActiveCamera(CameraNode& cam) { activeCamera = &cam; }

		// === Per-frame ===
		// Traverse graph, update world matrices, collect lights and probes
		void Update();

		// Build render queue for a given pass type (filters by visibility + frustum)
		gr::RenderQueue BuildRenderQueue(const math::Frustum& frustum, RenderPassType passType);

		// === Shadow pass ===
		void RenderShadowPass(gr::RenderQueue& queue, LightNode& light, const gpu::program::ShaderProgramPtr& depthShader);

		// === Opaque pass ===
		void RenderOpaquePass(gr::RenderQueue& queue, const gpu::program::ShaderProgramPtr& blinnPhongShader);

		// === Transparent pass ===
		void RenderTransparentPass(gr::RenderQueue& queue, const gpu::program::ShaderProgramPtr& blinnPhongShader);

		// === Reflection pass ===
		void RenderReflectionPass(ReflectionProbeNode& probe, const gpu::program::ShaderProgramPtr& shader);

		// === Uniform upload ===
		void UploadLights(const gpu::program::ShaderProgramPtr& shader);
		void UploadCamera(const gpu::program::ShaderProgramPtr& shader);
		void UploadModel(const gpu::program::ShaderProgramPtr& shader, const glm::mat4& model, const glm::mat3& normalMatrix);
		void UploadShadowMap(const gpu::program::ShaderProgramPtr& shader);

		// === Helpers ===
		glm::vec3 GetActiveCameraPosition() const;

		std::unique_ptr<SceneNode> root;

		CameraNode* activeCamera = nullptr;
		std::vector<LightNode*>        lights;          // non-owning, collected during update
		std::vector<ReflectionProbeNode*> reflectionProbes;

		// Options
		bool wireframeMode = false;
		bool enableFrustumCulling = true;
		bool enableInstancing = true;
		bool enableShadows = true;

		RenderStats lastFrameStats;

		gr::ShadowMapManager shadowMaps;

	private:
		// Draw a single render item
		void drawRenderItem(const gr::RenderItem& item, const gpu::program::ShaderProgramPtr& shader, bool receiveShadowUniform);

		// Rasterization state for wireframe
		gpu::RasterizationState m_wireframeState;
		gpu::RasterizationState m_fillState;

		gpu::texture::SamplerPtr m_shadowSampler;
		static constexpr uint32_t SHADOW_TEX_UNIT = 5;

		// SSBO for instance transforms (binding 6 in shaders)
		gpu::buffer::BufferPtr m_instanceSSBO;
		uint32_t m_instanceCapacity = 0;
		static constexpr uint32_t INSTANCE_SSBO_BINDING = 6;
		static constexpr uint32_t INITIAL_INSTANCE_CAPACITY = 1024;

		// UBO for lights (binding 4)
		gpu::buffer::BufferPtr m_lightUBO;
		static constexpr uint32_t LIGHT_UBO_BINDING = 4;
	};
} //namespace scene