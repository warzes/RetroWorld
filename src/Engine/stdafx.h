#pragma once

#include <3rdpartyConfig.h>
#include "EngineConfig.h"

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4820)
#	pragma warning(push, 3)
#	pragma warning(disable : 4865)
#	pragma warning(disable : 5039)
#endif

#define _USE_MATH_DEFINES

#if defined(_WIN32)

#	define WINVER 0x0A00 // windows 10
#	define _WIN32_WINNT	0x0A00 // windows 10

#	define NOGDICAPMASKS
#	define NOVIRTUALKEYCODES
//#	define NOWINMESSAGES
//#	define NOWINSTYLES
#	define NOSYSMETRICS
#	define NOMENUS
#	define NOICONS
#	define NOKEYSTATES
#	define NOSYSCOMMANDS
#	define NORASTEROPS
//#	define NOSHOWWINDOW
#	define OEMRESOURCE
#	define NOATOM
#	define NOCLIPBOARD
#	define NOCOLOR
#	define NOCTLMGR
#	define NODRAWTEXT
//#	define NOGDI
#	define NOKERNEL
//#	define NOUSER
#	define NONLS
#	define NOMB
#	define NOMEMMGR
#	define NOMETAFILE
#	define NOMINMAX
//#	define NOMSG
#	define NOOPENFILE
#	define NOSCROLL
#	define NOSERVICE
#	define NOSOUND
#	define NOTEXTMETRIC
#	define NOWH
#	define NOWINOFFSETS
#	define NOCOMM
#	define NOKANJI
#	define NOHELP
#	define NOPROFILER
#	define NODEFERWINDOWPOS
#	define NOMCX
#	define WIN32_LEAN_AND_MEAN

#	include <Windows.h>
#endif // defined(_WIN32)

#if USE_OPENGL
#	include <glad/gl.h>
#	if defined(_WIN32)
#		include <glad/wgl.h>
#	endif
#endif // USE_OPENGL

#include <cstdint>
#include <cassert>
#include <cmath>

#include <chrono>
#include <regex>
#include <thread>
#include <filesystem>
#include <fstream>
#include <string>
#include <array>
#include <vector>
#include <span>
#include <unordered_map>

#define GLM_FORCE_XYZW_ONLY 1
#define GLM_ENABLE_EXPERIMENTAL 1
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/dual_quaternion.hpp>
#include <glm/gtx/type_aligned.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/hash.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_win32.h>

#include <stb/stb_image.h>
#include <stb/stb_truetype.h>

#include <tiny_obj_loader/tiny_obj_loader.h>

#include <cgltf/cgltf.h>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif