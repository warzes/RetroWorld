#pragma once

#include "gpu_texture.h"
#include "gpu_program.h"

namespace gr
{
	class Material final
	{
	public:
		void Close();

		// Bind material uniforms + textures for a given shader program
		void Bind(const gpu::program::ShaderProgramPtr& shader);

		// Derived: true if opacity < 1 or albedoMap has alpha
		bool IsTransparent() const { return opacity < 1.0f; }

		glm::vec3 albedoColor = glm::vec3(1.0f);
		glm::vec3 specularColor = glm::vec3(1.0f);
		glm::vec3 ambientColor = glm::vec3(0.1f);
		float     shininess = 32.0f;
		float     opacity = 1.0f;

		// Texture slots (nullptr = no texture)
		gpu::texture::TexturePtr albedoMap;
		gpu::texture::TexturePtr normalMap;
		gpu::texture::TexturePtr specularMap;
		gpu::texture::TexturePtr emissiveMap;

		// Shared sampler for all texture slots (created lazily)
		gpu::texture::SamplerPtr sampler;

		// Flags
		bool castShadow = true;
		bool receiveShadow = true;
		bool isWallAtlas = false;

		// Per-material rasterisation state override
		gpu::CullMode cullMode = gpu::CullMode::Back;
	};
} // namespace gr