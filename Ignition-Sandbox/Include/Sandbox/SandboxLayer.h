#pragma once

#include "Ignition/Core/Layer.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/UI/ImGuiAccess.h"

#include <imgui.h>

namespace Sandbox
{
	class SandboxLayer : public Ignition::Layer
	{
	public:
		SandboxLayer(Ignition::Renderer* renderer, Ignition::Material* material) : m_Renderer(renderer), m_Material(material)
		{

		}

		void OnAttach() override
		{
			void* (*allocFunc)(size_t, void*) = nullptr;
			void (*freeFunc)(void*, void*) = nullptr;
			void* userData = nullptr;

			Ignition::UI::GetImGuiAllocatorFunctions(&allocFunc, &freeFunc, &userData);

			ImGui::SetAllocatorFunctions(allocFunc, freeFunc, userData);
			ImGui::SetCurrentContext(Ignition::UI::GetImGuiContext());
		}

		void OnRender() override
		{
			if (!m_Renderer->IsImGuiFrameActive())
			{
				return;
			}

			ImGui::Begin("Sandbox");
			ImGui::ColorEdit4("Quad Tint", &m_Material->Tint.x);
			ImGui::End();
		}

	private:
		Ignition::Renderer* m_Renderer = nullptr;
		Ignition::Material* m_Material = nullptr;
	};
}