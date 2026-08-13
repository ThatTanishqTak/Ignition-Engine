#include "Sandbox/ControlPanelLayer.h"

#include <Ignition/Core/Time.h>
#include <Ignition/Renderer/Renderer.h>
#include <Ignition/Renderer/Mesh.h>
#include <Ignition/Renderer/Vertex.h>
#include <Ignition/Scene/Scene.h>
#include <Ignition/Scene/Entity.h>
#include <Ignition/Scene/Components.h>
#include <Ignition/UI/UI.h>

#include <glm/vec3.hpp>

#include <memory>
#include <string>

namespace Sandbox
{
	namespace
	{
		constexpr size_t FrameHistorySize = 240;
	}

	ControlPanelLayer::ControlPanelLayer(Ignition::Renderer* renderer, Ignition::Scene* scene) : m_Renderer(renderer), m_Scene(scene)
	{
		m_FrameTimeHistory.reserve(FrameHistorySize);
	}

	void ControlPanelLayer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;

		if (m_FrameTimeHistory.size() >= FrameHistorySize)
		{
			m_FrameTimeHistory.erase(m_FrameTimeHistory.begin());
		}

		m_FrameTimeHistory.push_back(Ignition::Time::GetUnscaledDeltaTime().GetMilliseconds());
	}

	void ControlPanelLayer::OnRender()
	{
		Ignition::UI::SetNextWindowSize(360.0f, 560.0f);

		if (Ignition::UI::BeginWindow("Control Panel"))
		{
			DrawSessionSection();
			DrawSceneSection();
		}

		Ignition::UI::EndWindow();
	}

	void ControlPanelLayer::DrawSessionSection()
	{
		if (!Ignition::UI::CollapsingHeader("Session"))
		{
			return;
		}

		Ignition::UI::Text("FPS: {:.1f}", Ignition::Time::GetFramesPerSecond());
		Ignition::UI::Text("Frame Time: {:.2f} ms", Ignition::Time::GetAverageFrameTimeMilliseconds());
		Ignition::UI::Text("Entities: {}", m_Scene->GetEntities().size());

		if (!m_FrameTimeHistory.empty())
		{
			Ignition::UI::PlotLines("##FrameTimes", m_FrameTimeHistory.data(), static_cast<int>(m_FrameTimeHistory.size()), 0.0f, 33.3f, 60.0f, "Frame time (ms)");
		}

		Ignition::UI::Spacing();
	}

	void ControlPanelLayer::DrawSceneSection()
	{
		if (!Ignition::UI::CollapsingHeader("Scene"))
		{
			return;
		}

		if (Ignition::UI::Button("Spawn"))
		{
			SpawnEntity();
		}

		Ignition::UI::Separator();

		Ignition::Entity pendingDestroy{};

		for (Ignition::Entity entity : m_Scene->GetEntities())
		{
			Ignition::UI::PushID(static_cast<int>(entity.GetID()));

			if (Ignition::UI::SmallButton("Destroy"))
			{
				pendingDestroy = entity;
			}

			Ignition::UI::SameLine();

			if (Ignition::MeshRendererComponent* meshRenderer = entity.GetMeshRenderer())
			{
				Ignition::UI::ColorEdit4("##Tint", meshRenderer->Material.Tint);
				Ignition::UI::SameLine();
				Ignition::UI::Checkbox("##TwoSided", &meshRenderer->Material.TwoSided);
				Ignition::UI::SameLine();
			}

			Ignition::UI::Text(entity.GetName().c_str());

			Ignition::UI::PopID();
		}

		if (pendingDestroy.IsValid())
		{
			m_Scene->DestroyEntity(pendingDestroy);
		}
	}

	void ControlPanelLayer::SpawnEntity()
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
}