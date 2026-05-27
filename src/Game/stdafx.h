#pragma once

#include <3rdpartyConfig.h>
#include <EngineConfig.h>

#include <Engine/stdafx.h>

#if defined(_MSC_VER)
#	pragma warning(push, 3)
//#	pragma warning(disable : 4061)
#endif

#include <cstdint>
#include <filesystem>
#include <string>

#define GLM_FORCE_XYZW_ONLY 1
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>

#include <core_log.h>
#include <core_file.h>
#include <core_image.h>
#include <core_utils.h>

#include <math_core.h>
#include <math_point2.h>
#include <math_aabb.h>

#include <gpu_core.h>
#include <gpu_program.h>
#include <gpu_uniform.h>
#include <gpu_buffer.h>
#include <gpu_vao.h>
#include <gpu_texture.h>
#include <gpu_framebuffer.h>
#include <gpu_cmd.h>
#include <gpu_system.h>

#include <app.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif