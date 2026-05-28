#include "stdafx.h"
#include "gr_shadowMapManager.h"
#include "sc_lightNode.h"
//=============================================================================
gr::ShadowMap& gr::ShadowMapManager::GetOrCreate(scene::LightNode* light)
{
	auto it = m_maps.find(light);
	if (it != m_maps.end())
		return it->second;

	ShadowMap sm;
	sm.resolution = light->shadowSettings.resolution;

	if (light->lightType == scene::LightNode::LightType::Point)
	{
		// Cubemap depth texture for point light
		gpu::texture::TextureCreateInfo texInfo;
		texInfo.imageType = gpu::ImageType::TextureCubemap;
		texInfo.format = gpu::Format::D32_FLOAT;
		texInfo.extent = { (unsigned)sm.resolution, (unsigned)sm.resolution, 1 };
		texInfo.mipLevels = 1;
		texInfo.arrayLayers = 6;
		texInfo.sampleCount = gpu::SampleCount::Samples1;
		sm.isCubemap = true;
		sm.cubemapTexture = gpu::texture::CreateTexture(texInfo, "shadow_cubemap");

		gpu::fbo::FramebufferCreateInfo fboInfo;
		fboInfo.viewport = gpu::Viewport{
			.drawRect = {.offset = {0, 0}, .extent = {(unsigned)sm.resolution, (unsigned)sm.resolution} }
		};
		fboInfo.depthAttachment = gpu::fbo::RenderDepthStencilAttachment{
			.texture = sm.cubemapTexture,
			.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
			.clearValue = {.depth = 1.0f },
		};
		sm.framebuffer = gpu::fbo::CreateFramebuffer(fboInfo);
	}
	else
	{
		// 2D depth texture for directional / spot
		auto depthTex = gpu::texture::CreateTexture2D(
			{ (unsigned)sm.resolution, (unsigned)sm.resolution },
			gpu::Format::D32_FLOAT,
			"shadow_depth");

		gpu::fbo::FramebufferCreateInfo fboInfo;
		fboInfo.viewport = gpu::Viewport{
		.drawRect = {.offset = {0, 0}, .extent = {(unsigned)sm.resolution, (unsigned)sm.resolution} }
		};
		fboInfo.depthAttachment = gpu::fbo::RenderDepthStencilAttachment{
		.texture = depthTex,
		.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
		.clearValue = {.depth = 1.0f },
		};
		sm.framebuffer = gpu::fbo::CreateFramebuffer(fboInfo);
		sm.depthTexture = depthTex;
	}

	auto [newIt, _] = m_maps.emplace(light, std::move(sm));
	return newIt->second;
}
//=============================================================================
void gr::ShadowMapManager::Clear()
{
	m_maps.clear();
}
//=============================================================================