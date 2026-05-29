#include "stdafx.h"
#include "_res_loader.h"
#include "res_textureLoader.h"
#include "res_modelLoader.h"
//=============================================================================
extern std::unordered_map<std::string, gpu::texture::TexturePtr> texturesCache;
//=============================================================================
bool res::Init()
{
	return true;
}
//=============================================================================
void res::Close()
{
	texturesCache.clear();
}
//=============================================================================