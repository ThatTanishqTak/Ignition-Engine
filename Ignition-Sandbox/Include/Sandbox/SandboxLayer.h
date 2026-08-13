#pragma once

#include "Ignition/Core/Layer.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Vertex.h"
#include "Ignition/Scene/Scene.h"
#include "Ignition/UI/ImGuiAccess.h"

#include <imgui.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Sandbox
{
	class SandboxLayer : public Ignition::Layer
	{
	public:
		SandboxLayer(Ignition::Renderer* renderer, Ignition::Scene* scene) : m_Renderer(renderer), m_Scene(scene)
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

			ImGui::Begin("Scene");

			if (ImGui::Button("Spawn"))
			{
				SpawnEntity();
			}

			ImGui::Separator();

			Ignition::Entity pendingDestroy{};

			for (Ignition::Entity entity : m_Scene->GetEntities())
			{
				ImGui::PushID(static_cast<int>(entity.GetID()));

				if (ImGui::Button("Destroy"))
				{
					pendingDestroy = entity;
				}

				ImGui::SameLine();

				if (Ignition::MeshRendererComponent* meshRenderer = entity.GetMeshRenderer())
				{
					ImGui::ColorEdit4("##Tint", &meshRenderer->Material.Tint.x, ImGuiColorEditFlags_NoInputs);
					ImGui::SameLine();
					ImGui::Checkbox("##TwoSided", &meshRenderer->Material.TwoSided);
					ImGui::SameLine();
				}

				ImGui::TextUnformatted(entity.GetName().c_str());

				ImGui::PopID();
			}

			if (pendingDestroy.IsValid())
			{
				m_Scene->DestroyEntity(pendingDestroy);
			}

			ImGui::End();
		}

	private:
		void SpawnEntity()
		{
			const std::vector<Ignition::Vertex> vertices = {
				{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			};

			const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0, 2, 1, 0, 0, 3, 2 };
			const std::shared_ptr<Ignition::Mesh> mesh = m_Renderer->CreateMesh(vertices, indices);

			if (!mesh)
			{
				return;
			}

			++m_SpawnCount;

			Ignition::Entity entity = m_Scene->CreateEntity("Spawned " + std::to_string(m_SpawnCount));

			Ignition::TransformComponent& transform = entity.GetTransform();
			transform.Position = { -1.5f + 0.3f * static_cast<float>((m_SpawnCount - 1) % 11), -2.25f, 0.5f };
			transform.Scale = glm::vec3(0.25f);

			entity.AddMeshRenderer(mesh, {});
		}

	private:
		Ignition::Renderer* m_Renderer = nullptr;
		Ignition::Scene* m_Scene = nullptr;
		uint32_t m_SpawnCount = 0;
	};
}