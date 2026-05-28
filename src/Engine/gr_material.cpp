#include "stdafx.h"
#include "gr_material.h"
#include "gpu_cmd.h"
//=============================================================================
void gr::Material::Close()
{
	sampler.reset();
}
//=============================================================================
void gr::Material::Bind(const gpu::program::ShaderProgramPtr& shader)
{
	if (!shader) return;

	// Create default sampler if needed
	if (!sampler)
	{
		gpu::texture::SamplerState ss;
		ss.minFilter = gpu::Filter::Nearest;
		ss.magFilter = gpu::Filter::Nearest;
		ss.addressModeU = gpu::AddressMode::Repeat;
		ss.addressModeV = gpu::AddressMode::Repeat;
		sampler = gpu::texture::CreateSampler(ss);
	}

	// Material color uniforms
	auto locAlbedo = gpu::program::GetUniformLocation(shader, "u_albedoColor");
	auto locSpecular = gpu::program::GetUniformLocation(shader, "u_specularColor");
	auto locAmbient = gpu::program::GetUniformLocation(shader, "u_ambientColor");
	auto locShininess = gpu::program::GetUniformLocation(shader, "u_shininess");
	auto locOpacity = gpu::program::GetUniformLocation(shader, "u_opacity");

	gpu::program::SetUniform(shader, locAlbedo, albedoColor);
	gpu::program::SetUniform(shader, locSpecular, specularColor);
	gpu::program::SetUniform(shader, locAmbient, ambientColor);
	gpu::program::SetUniform(shader, locShininess, shininess);
	gpu::program::SetUniform(shader, locOpacity, opacity);

	// Texture bindings
	int texUnit = 0;

	auto bindTex = [&](const gpu::texture::TexturePtr& tex, const char* uniformName, const char* hasUniform)
		{
			int loc = gpu::program::GetUniformLocation(shader, uniformName);
			int locHas = gpu::program::GetUniformLocation(shader, hasUniform);
			if (tex)
			{
				gpu::program::SetUniform(shader, loc, texUnit);
				gpu::cmd::BindSampledImage(static_cast<uint32_t>(texUnit), tex, sampler);
				gpu::program::SetUniform(shader, locHas, true);
				++texUnit;
			}
			else
			{
				gpu::program::SetUniform(shader, locHas, false);
			}
		};

	bindTex(albedoMap, "u_albedoMap", "u_hasAlbedoMap");
	bindTex(normalMap, "u_normalMap", "u_hasNormalMap");
	bindTex(specularMap, "u_specularMap", "u_hasSpecularMap");
	bindTex(emissiveMap, "u_emissiveMap", "u_hasEmissiveMap");
}
//=============================================================================