#include "Editor/EditorLayer.h"

#include "Ignition/Ignition.h"

#include <imgui.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <array>
#include <cstring>

namespace Editor
{
	namespace
	{
		constexpr size_t FrameHistorySize = 240;
		constexpr const char* DefaultScenePath = "Assets/Scenes/Scene.yaml";
	}

	EditorLayer::EditorLayer(EditorContext* context, Ignition::Camera* camera, Ignition::AssetRegistry* assets, Ignition::Renderer* renderer, Ignition::Input* input) : m_Context(context), m_Camera(camera), m_Assets(assets), m_Renderer(renderer), m_Input(input), m_GizmoOperation(ImGuizmo::TRANSLATE)
	{
		m_FrameTimeHistory.reserve(FrameHistorySize);
	}

	void EditorLayer::OnAttach()
	{
		void* allocateFunction = nullptr;
		void* freeFunction = nullptr;
		void* userData = nullptr;
		Ignition::UI::GetAllocatorFunctions(allocateFunction, freeFunction, userData);

		ImGui::SetAllocatorFunctions(reinterpret_cast<ImGuiMemAllocFunc>(allocateFunction), reinterpret_cast<ImGuiMemFreeFunc>(freeFunction), userData);
		ImGui::SetCurrentContext(static_cast<ImGuiContext*>(Ignition::UI::GetContext()));
	}

	void EditorLayer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;

		if (m_FrameTimeHistory.size() >= FrameHistorySize)
		{
			m_FrameTimeHistory.erase(m_FrameTimeHistory.begin());
		}

		m_FrameTimeHistory.push_back(Ignition::Time::GetUnscaledDeltaTime().GetMilliseconds());

		// Ctrl+S / Ctrl+O work regardless of which panel has focus
		if (m_Input->IsKeyDown(Ignition::ScanCode::LCTRL))
		{
			if (m_Input->IsKeyPressed(Ignition::ScanCode::S))
			{
				SaveScene(m_Context->ScenePath.empty() ? DefaultScenePath : m_Context->ScenePath);
			}
			else if (m_Input->IsKeyPressed(Ignition::ScanCode::O))
			{
				PromptForPath(PathPrompt::Open);
			}
		}

		if ((m_Context->ViewportHovered || m_Context->ViewportFocused) && !Ignition::UI::WantsTextInput() && !m_Input->IsMouseButtonDown(Ignition::MouseCode::RIGHT))
		{
			if (m_Input->IsKeyPressed(Ignition::ScanCode::W))
			{
				m_GizmoOperation = ImGuizmo::TRANSLATE;
			}
			
			if (m_Input->IsKeyPressed(Ignition::ScanCode::E))
			{
				m_GizmoOperation = ImGuizmo::ROTATE;
			}
			
			if (m_Input->IsKeyPressed(Ignition::ScanCode::R))
			{
				m_GizmoOperation = ImGuizmo::SCALE;
			}
			
			if (m_Input->IsKeyPressed(Ignition::ScanCode::X))
			{
				m_GizmoWorldSpace = !m_GizmoWorldSpace;
			}
		}
	}

	void EditorLayer::OnRender()
	{
		if (!Ignition::UI::IsFrameActive())
		{
			return;
		}

		ImGuizmo::BeginFrame();

		const unsigned int dockspaceID = Ignition::UI::DockSpaceOverMainViewport();

		if (!m_DockLayoutInitialized)
		{
			m_DockLayoutInitialized = true;
			Ignition::UI::BuildDefaultDockLayout(dockspaceID, "Hierarchy", "Inspector", "Stats", "Viewport");
		}

		DrawMenuBar();
		DrawPathPrompt();
		DrawViewportPanel();
		DrawHierarchyPanel();
		DrawInspectorPanel();
		DrawStatsPanel();
	}

	void EditorLayer::DrawMenuBar()
	{
		if (!Ignition::UI::BeginMainMenuBar())
		{
			return;
		}

		if (Ignition::UI::BeginMenu("File"))
		{
			if (Ignition::UI::MenuItem("New Scene"))
			{
				NewScene();
			}

			if (Ignition::UI::MenuItem("Open Scene...", "Ctrl+O"))
			{
				PromptForPath(PathPrompt::Open);
			}

			if (Ignition::UI::MenuItem("Save Scene", "Ctrl+S"))
			{
				SaveScene(m_Context->ScenePath.empty() ? DefaultScenePath : m_Context->ScenePath);
			}

			if (Ignition::UI::MenuItem("Save Scene As..."))
			{
				PromptForPath(PathPrompt::SaveAs);
			}

			Ignition::UI::EndMenu();
		}

		Ignition::UI::EndMainMenuBar();
	}

	void EditorLayer::DrawViewportPanel()
	{
		Ignition::UI::PushWindowPadding(0.0f, 0.0f);

		if (Ignition::UI::BeginWindow("Viewport"))
		{
			m_Context->ViewportHovered = Ignition::UI::IsWindowHovered();
			m_Context->ViewportFocused = Ignition::UI::IsWindowFocused();

			m_Context->ViewportPosition = Ignition::UI::GetCursorScreenPosition();
			m_Context->ViewportSize = Ignition::UI::GetContentRegionAvailable();

			const uint32_t targetWidth = m_Context->ViewportSize.x > 0.0f ? static_cast<uint32_t>(m_Context->ViewportSize.x) : 0;
			const uint32_t targetHeight = m_Context->ViewportSize.y > 0.0f ? static_cast<uint32_t>(m_Context->ViewportSize.y) : 0;

			if (targetWidth > 0 && targetHeight > 0)
			{
				// The renderer defers the actual swap to the start of the next frame
				m_Renderer->SetSceneRenderTargetSize(targetWidth, targetHeight);
				m_Camera->SetAspectRatio(static_cast<float>(targetWidth) / static_cast<float>(targetHeight));
			}

			Ignition::UI::Image(m_Renderer->GetSceneRenderTargetTextureID(), m_Context->ViewportSize.x, m_Context->ViewportSize.y);

			DrawGizmo();
		}

		Ignition::UI::EndWindow();
		Ignition::UI::PopStyleVariable();
	}

	void EditorLayer::DrawGizmo()
	{
		m_Context->GizmoUsing = false;

		if (!m_Context->Selection.IsValid() || m_Context->ViewportSize.x <= 0.0f || m_Context->ViewportSize.y <= 0.0f)
		{
			return;
		}

		ImGuizmo::SetOrthographic(false);
		ImGuizmo::SetDrawlist();
		ImGuizmo::SetRect(m_Context->ViewportPosition.x, m_Context->ViewportPosition.y, m_Context->ViewportSize.x, m_Context->ViewportSize.y);

		Ignition::TransformComponent& transform = m_Context->Selection.GetTransform();
		glm::mat4 transformMatrix = transform.GetMatrix();

		// Hold Ctrl to snap: 0.5 m translate, 15 degrees rotate
		const bool snap = m_Input->IsKeyDown(Ignition::ScanCode::LCTRL);
		const float snapValue = m_GizmoOperation == ImGuizmo::ROTATE ? 15.0f : 0.5f;
		const std::array<float, 3> snapValues = { snapValue, snapValue, snapValue };

		const ImGuizmo::MODE mode = (m_GizmoWorldSpace && m_GizmoOperation != ImGuizmo::SCALE) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

		ImGuizmo::Manipulate(glm::value_ptr(m_Camera->GetView()), glm::value_ptr(m_Camera->GetProjection()), static_cast<ImGuizmo::OPERATION>(m_GizmoOperation), mode, glm::value_ptr(transformMatrix), nullptr, snap ? snapValues.data() : nullptr);

		// Suppress camera input while the gizmo is hot
		m_Context->GizmoUsing = ImGuizmo::IsUsing() || ImGuizmo::IsOver();

		if (ImGuizmo::IsUsing())
		{
			std::array<float, 3> translation{};
			std::array<float, 3> rotationDegrees{};
			std::array<float, 3> scale{};

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(transformMatrix), translation.data(), rotationDegrees.data(), scale.data());

			transform.Position = { translation[0], translation[1], translation[2] };
			transform.Rotation = glm::radians(glm::vec3(rotationDegrees[0], rotationDegrees[1], rotationDegrees[2]));
			transform.Scale = { scale[0], scale[1], scale[2] };
		}
	}

	void EditorLayer::DrawHierarchyPanel()
	{
		if (Ignition::UI::BeginWindow("Hierarchy"))
		{
			Ignition::Entity deferredDelete{};

			for (Ignition::Entity entity : m_Context->Scene->GetEntities())
			{
				Ignition::UI::PushID(static_cast<int>(entity.GetID()));

				if (Ignition::UI::Selectable(entity.GetName().c_str(), m_Context->Selection == entity))
				{
					m_Context->Selection = entity;
				}

				if (Ignition::UI::BeginPopupContextItem("EntityContext"))
				{
					if (Ignition::UI::MenuItem("Duplicate"))
					{
						m_Context->Selection = m_Context->Scene->DuplicateEntity(entity);
					}

					if (Ignition::UI::MenuItem("Delete"))
					{
						deferredDelete = entity;
					}

					Ignition::UI::EndPopup();
				}

				Ignition::UI::PopID();
			}

			if (Ignition::UI::BeginPopupContextWindow("HierarchyContext"))
			{
				if (Ignition::UI::MenuItem("Create Empty"))
				{
					m_Context->Selection = m_Context->Scene->CreateEntity("Entity");
				}

				Ignition::UI::EndPopup();
			}

			if (deferredDelete.IsValid())
			{
				if (m_Context->Selection == deferredDelete)
				{
					m_Context->Selection = {};
				}

				m_Context->Scene->DestroyEntity(deferredDelete);
			}
		}

		Ignition::UI::EndWindow();
	}

	void EditorLayer::DrawInspectorPanel()
	{
		if (Ignition::UI::BeginWindow("Inspector"))
		{
			if (!m_Context->Selection.IsValid())
			{
				Ignition::UI::TextDisabled("No entity selected");
				Ignition::UI::EndWindow();

				return;
			}

			Ignition::Entity entity = m_Context->Selection;

			std::array<char, 256> nameBuffer{};
			const std::string& entityName = entity.GetName();
			std::memcpy(nameBuffer.data(), entityName.c_str(), std::min(entityName.size(), nameBuffer.size() - 1));

			if (Ignition::UI::InputText("Name", nameBuffer.data(), nameBuffer.size()))
			{
				entity.SetName(nameBuffer.data());
			}

			if (Ignition::UI::CollapsingHeader("Transform"))
			{
				Ignition::TransformComponent& transform = entity.GetTransform();

				Ignition::UI::DragFloat3("Position", transform.Position, 0.01f);

				// Radians in the component, degrees at the UI boundary only
				glm::vec3 rotationDegrees = glm::degrees(transform.Rotation);

				if (Ignition::UI::DragFloat3("Rotation", rotationDegrees, 0.5f))
				{
					transform.Rotation = glm::radians(rotationDegrees);
				}

				Ignition::UI::DragFloat3("Scale", transform.Scale, 0.01f);
			}

			if (Ignition::MeshRendererComponent* meshRenderer = entity.GetMeshRenderer())
			{
				if (Ignition::UI::CollapsingHeader("Mesh Renderer"))
				{
					Ignition::UI::LabelText("Mesh", meshRenderer->MeshAsset.empty() ? "<unreferenced>" : meshRenderer->MeshAsset.c_str());

					if (meshRenderer->Mesh)
					{
						const Ignition::MeshBounds bounds = meshRenderer->Mesh->GetBounds();
						const glm::vec3 size = (bounds.Maximum - bounds.Minimum) * entity.GetTransform().Scale;

						Ignition::UI::Text("Size: {:.3f} x {:.3f} x {:.3f} m", size.x, size.y, size.z);
					}

					Ignition::UI::ColorEdit4("Tint", meshRenderer->Material.Tint, true);
					Ignition::UI::Checkbox("Two Sided", &meshRenderer->Material.TwoSided);
				}
			}

			Ignition::UI::Separator();

			if (Ignition::UI::Button("Add Component"))
			{
				Ignition::UI::OpenPopup("AddComponent");
			}

			if (Ignition::UI::BeginPopup("AddComponent"))
			{
				if (!entity.GetMeshRenderer())
				{
					if (Ignition::UI::MenuItem("Mesh Renderer (Quad)"))
					{
						auto& meshRenderer = entity.AddMeshRenderer(m_Assets->LoadMesh("builtin:quad"));
						meshRenderer.MeshAsset = "builtin:quad";
					}

					if (Ignition::UI::MenuItem("Mesh Renderer (Cube)"))
					{
						auto& meshRenderer = entity.AddMeshRenderer(m_Assets->LoadMesh("builtin:cube"));
						meshRenderer.MeshAsset = "builtin:cube";
					}
				}

				Ignition::UI::EndPopup();
			}
		}

		Ignition::UI::EndWindow();
	}

	void EditorLayer::DrawStatsPanel()
	{
		if (Ignition::UI::BeginWindow("Stats"))
		{
			Ignition::UI::Text("FPS: {:.1f}", Ignition::Time::GetFramesPerSecond());
			Ignition::UI::Text("Frame Time: {:.2f} ms", Ignition::Time::GetAverageFrameTimeMilliseconds());
			Ignition::UI::Text("Entities: {}", m_Context->Scene->GetEntities().size());
			Ignition::UI::Text("Scene: {}", m_Context->ScenePath.empty() ? "<unsaved>" : m_Context->ScenePath);

			if (!m_FrameTimeHistory.empty())
			{
				Ignition::UI::PlotLines("##FrameTimes", m_FrameTimeHistory.data(), static_cast<int>(m_FrameTimeHistory.size()), 0.0f, 33.3f, 60.0f, "Frame time (ms)");
			}
		}

		Ignition::UI::EndWindow();
	}

	void EditorLayer::PromptForPath(PathPrompt prompt)
	{
		const std::string& seed = m_Context->ScenePath.empty() ? std::string(DefaultScenePath) : m_Context->ScenePath;

		m_PathBuffer.fill('\0');
		std::memcpy(m_PathBuffer.data(), seed.c_str(), std::min(seed.size(), m_PathBuffer.size() - 1));

		m_PathPrompt = prompt;
		m_PathPromptRequested = true;
	}

	void EditorLayer::DrawPathPrompt()
	{
		if (m_PathPromptRequested)
		{
			Ignition::UI::OpenPopup("ScenePath");
			m_PathPromptRequested = false;
		}

		if (!Ignition::UI::BeginPopup("ScenePath"))
		{
			return;
		}

		const bool saving = m_PathPrompt == PathPrompt::SaveAs;

		Ignition::UI::Text(saving ? "Save scene as" : "Open scene");
		Ignition::UI::InputText("##ScenePath", m_PathBuffer.data(), m_PathBuffer.size());

		if (Ignition::UI::Button(saving ? "Save" : "Open"))
		{
			const std::string filepath = m_PathBuffer.data();

			if (!filepath.empty())
			{
				if (saving)
				{
					SaveScene(filepath);
				}
				else
				{
					OpenScene(filepath);
				}
			}

			m_PathPrompt = PathPrompt::None;
			Ignition::UI::CloseCurrentPopup();
		}

		Ignition::UI::SameLine();

		if (Ignition::UI::Button("Cancel"))
		{
			m_PathPrompt = PathPrompt::None;
			Ignition::UI::CloseCurrentPopup();
		}

		Ignition::UI::EndPopup();
	}

	void EditorLayer::NewScene()
	{
		m_Context->Selection = {};

		for (Ignition::Entity entity : m_Context->Scene->GetEntities())
		{
			m_Context->Scene->DestroyEntity(entity);
		}

		m_Context->ScenePath.clear();
	}

	void EditorLayer::OpenScene(const std::string& filepath)
	{
		m_Context->Selection = {};

		const Ignition::SceneSerializer serializer(m_Context->Scene, m_Assets);

		if (serializer.Load(filepath))
		{
			m_Context->ScenePath = filepath;
			IG_APP_INFO("Opened scene '{}'", filepath);
		}
		else
		{
			IG_APP_ERROR("Failed to open scene '{}'", filepath);
		}
	}

	void EditorLayer::SaveScene(const std::string& filepath)
	{
		const Ignition::SceneSerializer serializer(m_Context->Scene, m_Assets);

		if (serializer.Save(filepath))
		{
			m_Context->ScenePath = filepath;
			IG_APP_INFO("Saved scene '{}'", filepath);
		}
		else
		{
			IG_APP_ERROR("Failed to save scene '{}'", filepath);
		}
	}
}