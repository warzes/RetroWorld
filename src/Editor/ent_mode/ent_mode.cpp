#include "stdafx.h"
#include "ent_mode.hpp"
#include "../assets.hpp"
#include "../editor_app.hpp"
#include "../dialogs/dialogs.hpp"
#include <imgui/imgui.h>
#include <glm/gtc/type_ptr.hpp>
//=============================================================================
ed::EntMode::~EntMode() = default;
//=============================================================================
ed::EntMode::EntMode()
	: _ent(2.0f)
{
	memset(_texturePathBuffer, 0, sizeof(_texturePathBuffer));
	memset(_modelPathBuffer, 0, sizeof(_modelPathBuffer));
	memset(_newKeyBuffer, 0, sizeof(_newKeyBuffer));
	memset(_newValBuffer, 0, sizeof(_newValBuffer));
}
//=============================================================================
void ed::EntMode::OnEnter()
{}
//=============================================================================
void ed::EntMode::OnExit()
{}
//=============================================================================
void ed::EntMode::Update()
{}
//=============================================================================
void ed::EntMode::Draw()
{
	ImGui::Begin("Entity Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::SeparatorText("Properties");
	ImGui::DragFloat("Radius", &_ent.radius, 0.1f, 0.0f, 100.0f);
	ImGui::ColorEdit3("Color", glm::value_ptr(_ent.color));
	ImGui::InputInt("Yaw", &_ent.yaw);
	ImGui::InputInt("Pitch", &_ent.pitch);

	const char* displayModes[] = { "SPHERE", "MODEL", "SPRITE" };
	int currentMode = static_cast<int>(_ent.display);
	if (ImGui::Combo("Display Mode", &currentMode, displayModes, IM_ARRAYSIZE(displayModes)))
	{
		_ent.display = static_cast<Ent::DisplayMode>(currentMode);
	}

	ImGui::SeparatorText("Paths");
	ImGui::InputText("Texture Path", _texturePathBuffer, sizeof(_texturePathBuffer));
	ImGui::InputText("Model Path", _modelPathBuffer, sizeof(_modelPathBuffer));

	if (ImGui::Button("Load Texture") && strlen(_texturePathBuffer) > 0)
	{
		_ent.texture = Assets::GetTexture(_texturePathBuffer);
	}
	ImGui::SameLine();
	if (ImGui::Button("Load Model") && strlen(_modelPathBuffer) > 0)
	{
		_ent.model = Assets::GetModel(_modelPathBuffer);
	}

	ImGui::SeparatorText("Custom Properties");

	if (ImGui::BeginTable("props", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("Key");
		ImGui::TableSetupColumn("Value");
		ImGui::TableSetupColumn("");
		ImGui::TableHeadersRow();

		std::string keyToRemove;

		for (auto it = _ent.properties.begin(); it != _ent.properties.end(); )
		{
			auto& key = it->first;
			auto& val = it->second;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::Text("%s", key.c_str());
			ImGui::TableNextColumn();

			char valBuf[256];
			strncpy_s(valBuf, val.c_str(), sizeof(valBuf) - 1);
			valBuf[sizeof(valBuf) - 1] = 0;

			if (ImGui::InputText(("##val_" + key).c_str(), valBuf, sizeof(valBuf), ImGuiInputTextFlags_EnterReturnsTrue))
			{
				_ent.properties[key] = valBuf;
			}

			ImGui::TableNextColumn();
			ImGui::PushID(key.c_str());
			if (ImGui::SmallButton("X"))
			{
				keyToRemove = key;
			}
			ImGui::PopID();

			if (!keyToRemove.empty())
			{
				_ent.properties.erase(keyToRemove);
				keyToRemove.clear();
				break;
			}

			++it;
		}

		ImGui::EndTable();
	}

	ImGui::Separator();
	ImGui::InputText("##newKey", _newKeyBuffer, sizeof(_newKeyBuffer));
	ImGui::SameLine();
	ImGui::InputText("##newVal", _newValBuffer, sizeof(_newValBuffer));
	ImGui::SameLine();

	if (ImGui::Button("Add Property") && strlen(_newKeyBuffer) > 0)
	{
		_ent.properties[_newKeyBuffer] = _newValBuffer;
		memset(_newKeyBuffer, 0, sizeof(_newKeyBuffer));
		memset(_newValBuffer, 0, sizeof(_newValBuffer));
	}

	ImGui::End();
}
//=============================================================================
