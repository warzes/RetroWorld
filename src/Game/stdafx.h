#pragma once

#include <3rdpartyConfig.h>
#include <EngineConfig.h>

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

#include <math_core.h>
#include <math_point2.h>
#include <math_aabb.h>

#include <app.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif