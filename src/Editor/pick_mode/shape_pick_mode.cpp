#include "stdafx.h"
#include "pick_mode.hpp"
#include "../assets.hpp"
#include "../editor_app.hpp"
#include "../place_mode/place_mode.hpp"
//=============================================================================
namespace
{
	constexpr uint32_t PREVIEW_SIZE = 128;

	// Must match std140 layout in blinnPhongFrag (sc_sceneManager.cpp)
	struct alignas(16) LightDataGPU
	{
		glm::vec4 positionOrDirection;
		glm::vec3 color;
		float     intensity;
		glm::vec3 attenuation;
		float     radius;
		glm::vec3 spotDirection;
		float     innerCutoff;
		float     outerCutoff;
		int32_t   type;
		int32_t   castShadow;
		float     shadowBias;
		glm::mat4 lightSpaceMatrix;
	};
	static_assert(sizeof(LightDataGPU) == 144, "LightDataGPU std140 size mismatch");

	struct alignas(16) LightBlockUBO
	{
		int32_t      lightCount;
		uint8_t      _pad[12];
		LightDataGPU lights[16];
	};
	static_assert(sizeof(LightBlockUBO) == 4 + 12 + 144 * 16, "LightBlockUBO std140 size mismatch");
}
//=============================================================================
ed::ShapePickMode::ShapePickMode()
	: PickMode(1, ".obj")
{
	_rootDir = EditorApp::Get().GetShapesDir();
}
//=============================================================================
void ed::ShapePickMode::OnEnter()
{
	PickMode::OnEnter();
	_previews.clear();
	_previewRotation = 0.0f;
	_nextCreateIndex = 0;
}
//=============================================================================
void ed::ShapePickMode::Update()
{
	PickMode::Update();

	// Advance rotation
	_previewRotation += app::GetDeltaTime() * 0.8f;

	// Lazy-create FBOs (up to 2 per frame)
	for (size_t created = 0; created < 2 && _nextCreateIndex < _frames.size(); ++_nextCreateIndex)
	{
		const auto& path = _frames[_nextCreateIndex].filePath;
		if (_previews.find(path) == _previews.end())
		{
			createPreviewFBO(path);
			++created;
		}
	}

	// Re-render all existing previews each frame with updated rotation
	for (auto& [path, entry] : _previews)
	{
		if (entry.fbo)
			renderPreview(path, _previewRotation);
	}
}
//=============================================================================
gpu::texture::TexturePtr ed::ShapePickMode::GetFrameTexture(const std::filesystem::path& filePath)
{
	auto it = _previews.find(filePath);
	if (it != _previews.end())
		return it->second.colorTex;
	return gpu::texture::TexturePtr();
}
//=============================================================================
void ed::ShapePickMode::SelectFrame(const Frame& frame)
{
	_selectedShape = Assets::GetModel(frame.filePath);

	auto& app = EditorApp::Get();
	app.ChangeEditorMode(EditorApp::Mode::PLACE_TILE);
	app.GetPlaceMode().SetShapeFromModel(frame.filePath);
}
//=============================================================================
bool ed::ShapePickMode::IsFrameSelected(const std::filesystem::path& filePath)
{
	return _selectedShape && _selectedShape->GetPath() == filePath;
}
//=============================================================================
void ed::ShapePickMode::createPreviewFBO(const std::filesystem::path& path)
{
	auto handle = Assets::GetModel(path);
	if (!handle)
	{
		_previews[path] = {};
		return;
	}

	auto mesh = handle->GetMesh();
	if (!mesh || !mesh->vao)
	{
		_previews[path] = {};
		return;
	}

	PreviewEntry entry;

	entry.colorTex = gpu::texture::CreateTexture2D(
		{ PREVIEW_SIZE, PREVIEW_SIZE },
		gpu::Format::R8G8B8A8_UNORM,
		"shape_preview_color");

	auto depthTex = gpu::texture::CreateTexture2D(
		{ PREVIEW_SIZE, PREVIEW_SIZE },
		gpu::Format::D32_FLOAT,
		"shape_preview_depth");

	gpu::fbo::FramebufferCreateInfo fboInfo{};
	fboInfo.colorAttachments.push_back({
		.texture = entry.colorTex,
		.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
		.clearValue = {0.08f, 0.08f, 0.08f, 1.0f},
	});
	fboInfo.depthAttachment = gpu::fbo::RenderDepthStencilAttachment{
		.texture = depthTex,
		.loadOp = gpu::fbo::AttachmentLoadOp::Clear,
		.clearValue = {1.0f, 0},
	};
	entry.fbo = gpu::fbo::CreateFramebuffer(fboInfo);
	_previews[path] = std::move(entry);
}
//=============================================================================
void ed::ShapePickMode::renderPreview(const std::filesystem::path& path, float angleRad)
{
	auto it = _previews.find(path);
	if (it == _previews.end() || !it->second.fbo)
		return;

	auto handle = Assets::GetModel(path);
	if (!handle) return;

	auto mesh = handle->GetMesh();
	if (!mesh || !mesh->vao) return;

	auto& entry = it->second;

	// Compute bounding sphere
	glm::vec3 center(0.0f);
	float radius = 1.0f;
	if (mesh->aabb.IsValid())
	{
		center = mesh->aabb.GetCenter();
		radius = glm::length(mesh->aabb.GetExtents());
		if (radius < 0.001f) radius = 0.5f;
	}

	// Camera orbit
	float camDist = radius * 3.5f;
	glm::vec3 camPos = center + glm::vec3(camDist, camDist * 0.6f, camDist);
	glm::mat4 view = glm::lookAt(camPos, center, glm::vec3(0.0f, 1.0f, 0.0f));

	float halfSize = radius * 2.0f;
	glm::mat4 proj = glm::ortho(-halfSize, halfSize, -halfSize, halfSize,
		-camDist * 2.0f, camDist * 2.0f);

	auto shader = ed::EditorApp::GetBlinnPhongProgram();
	if (!shader) return;

	// Begin offscreen render
	gpu::cmd::BeginDraw(entry.fbo, "ShapePreview");
	gpu::cmd::BindShaderProgram(shader);

	// Camera uniforms
	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_view"), view);
	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_projection"), proj);
	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_cameraPos"), camPos);

	// Model matrix: center then rotate around Y
	glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), angleRad, glm::vec3(0.0f, 1.0f, 0.0f));
	modelMatrix = glm::translate(modelMatrix, -center);
	glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_model"), modelMatrix);
	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_normalMatrix"), normalMatrix);
	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_isInstanced"), false);

	// Light UBO
	static auto g_lightUBO = gpu::buffer::CreateBuffer(
		sizeof(LightBlockUBO),
		gpu::buffer::BufferStorageFlag::DynamicStorage,
		"preview_light_ubo");

	LightBlockUBO block{};
	block.lightCount = 1;
	auto& light = block.lights[0];
	light.positionOrDirection = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	light.color = glm::vec3(1.0f, 0.95f, 0.85f);
	light.intensity = 1.2f;
	light.attenuation = glm::vec3(0.0f);
	light.radius = 0.0f;
	light.spotDirection = glm::normalize(glm::vec3(-0.5f, 0.7f, -0.5f));
	light.innerCutoff = 0.0f;
	light.outerCutoff = 0.0f;
	light.type = 0;
	light.castShadow = 0;
	light.shadowBias = 0.0f;
	light.lightSpaceMatrix = glm::mat4(1.0f);

	gpu::buffer::UpdateData(g_lightUBO, &block, sizeof(LightBlockUBO));
	gpu::cmd::BindUniformBuffer(4, g_lightUBO, 0, sizeof(LightBlockUBO));

	// Green wireframe material
	gr::Material mat;
	mat.albedoColor = glm::vec3(0.0f, 0.9f, 0.0f);
	mat.ambientColor = glm::vec3(1.0f);
	mat.specularColor = glm::vec3(0.0f);
	mat.shininess = 1.0f;
	mat.opacity = 1.0f;
	mat.cullMode = gpu::CullMode::None;
	mat.Bind(shader);

	gpu::program::SetUniform(shader,
		gpu::program::GetUniformLocation(shader, "u_receiveShadow"), false);

	// Wireframe rasterization mode
	gpu::RasterizationState wireState;
	wireState.polygonMode = gpu::PolygonMode::Line;
	wireState.cullMode = gpu::CullMode::None;
	gpu::cmd::SetState(wireState);

	// Draw
	mesh->Bind();
	mesh->Draw();

	// Restore fill state (important for subsequent render passes)
	gpu::RasterizationState fillState;
	fillState.polygonMode = gpu::PolygonMode::Fill;
	fillState.cullMode = gpu::CullMode::Back;
	gpu::cmd::SetState(fillState);

	gpu::cmd::EndDraw();
}
//=============================================================================