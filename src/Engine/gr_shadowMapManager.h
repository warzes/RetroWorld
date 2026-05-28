#pragma once

#include "gpu_framebuffer.h"

namespace scene
{
	class LightNode;
} // namespace scene

namespace gr
{
	struct ShadowMap final
	{
		gpu::fbo::FramebufferPtr framebuffer;
		gpu::texture::TexturePtr depthTexture;
		gpu::texture::TexturePtr cubemapTexture;
		bool                     isCubemap = false;
		int                      resolution = 1024;
		glm::mat4                lightSpaceMatrix = glm::mat4(1.0f);
		std::array<glm::mat4, 4> cascadeMatrices = {};
	};

	class ShadowMapManager final
	{
	public:
		// Get or create shadow map for a given light
		ShadowMap& GetOrCreate(scene::LightNode* light);

		// Release resources
		void Clear();

	private:
		std::unordered_map<scene::LightNode*, ShadowMap> m_maps;
	};

} //namespace gr