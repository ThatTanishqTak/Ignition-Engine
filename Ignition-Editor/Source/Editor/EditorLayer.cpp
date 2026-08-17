#include "Editor/EditorLayer.h"

#include "Ignition/Ignition.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>
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

		constexpr glm::vec4 ColliderColor{ 0.2f, 0.9f, 0.3f, 1.0f };
		constexpr glm::vec4 PlayBorderColor{ 1.0f, 0.55f, 0.1f, 1.0f };

		glm::mat4 ColliderTransform(const Ignition::TransformComponent& transform, const glm::vec3& offset)
		{
			const glm::mat4 rotation = glm::mat4_cast(transform.Rotation);

			return glm::translate(glm::mat4(1.0f), transform.Position + glm::vec3(rotation * glm::vec4(offset * transform.Scale, 0.0f))) * rotation;
		}

		float UniformScale(const glm::vec3& scale)
		{
			return glm::max(glm::max(std::abs(scale.x), std::abs(scale.y)), std::abs(scale.z));
		}
	}

	EditorLayer::EditorLayer(EditorContext* context, Ignition::Camera* camera, Ignition::AssetRegistry* assets, Ignition::Renderer* renderer, Ignition::Input* input) : m_Context(context), m_Camera(camera), m_Assets(assets), m_Renderer(renderer), m_Input(input)
	{
		m_FrameTimeHistory.reserve(FrameHistorySize);
	}

	void EditorLayer::OnAttach()
	{
		// Edit-mode raycasts need a live PxScene, so the query world is created up front and kept alive
		Ignition::PhysicsSettings settings{};
		settings.QueryOnly = true;

		m_EditorPhysics = std::make_unique<Ignition::PhysicsWorld>(m_Context->Scene, m_Assets, settings);
		m_Context->PhysicsSceneDirty = true;
	}

	void EditorLayer::OnFixedUpdate(float fixedTimeStep)
	{
		if (!m_RuntimePhysics)
		{
			return;
		}

		if (m_Context->Play == PlayState::Playing)
		{
			m_RuntimePhysics->Step(fixedTimeStep);
		}
		else if (m_Context->Play == PlayState::Paused && m_Context->StepRequested)
		{
			m_RuntimePhysics->Step(fixedTimeStep);
			m_Context->StepRequested = false;
		}
	}

	void EditorLayer::OnUpdate(float deltaTime)
	{
		(void)deltaTime;

		if (m_FrameTimeHistory.size() >= FrameHistorySize)
		{
			m_FrameTimeHistory.erase(m_FrameTimeHistory.begin());
		}

		m_FrameTimeHistory.push_back(Ignition::Time::GetUnscaledDeltaTime().GetMilliseconds());

		if (m_RuntimePhysics)
		{
			// 120 Hz physics rendered smoothly at whatever the display rate happens to be
			m_RuntimePhysics->SyncTransforms(m_Context->Play == PlayState::Playing ? Ignition::Time::GetFixedAlpha() : 1.0f);
		}
		else
		{
			RefreshEditorPhysics();
		}

		if (m_Context->ViewportHovered && !m_Context->GizmoUsing && m_Input->IsMouseButtonPressed(Ignition::MouseCode::LEFT))
		{
			PickEntityUnderCursor();
		}

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
				m_GizmoOperation = Ignition::UI::GizmoOperation::Translate;
			}

			if (m_Input->IsKeyPressed(Ignition::ScanCode::E))
			{
				m_GizmoOperation = Ignition::UI::GizmoOperation::Rotate;
			}

			if (m_Input->IsKeyPressed(Ignition::ScanCode::R))
			{
				m_GizmoOperation = Ignition::UI::GizmoOperation::Scale;
			}

			if (m_Input->IsKeyPressed(Ignition::ScanCode::X))
			{
				m_GizmoWorldSpace = !m_GizmoWorldSpace;
			}
		}
	}

	Ignition::PhysicsWorld* EditorLayer::GetActivePhysicsWorld() const
	{
		return m_RuntimePhysics ? m_RuntimePhysics.get() : m_EditorPhysics.get();
	}

	void EditorLayer::RefreshEditorPhysics()
	{
		if (!m_EditorPhysics || !m_Context->PhysicsSceneDirty)
		{
			return;
		}

		m_EditorPhysics->Rebuild();
		m_Context->PhysicsSceneDirty = false;
	}

	void EditorLayer::PickEntityUnderCursor()
	{
		Ignition::PhysicsWorld* physics = GetActivePhysicsWorld();

		if (!physics || !physics->IsValid() || m_Context->ViewportSize.x <= 0.0f || m_Context->ViewportSize.y <= 0.0f)
		{
			return;
		}

		const glm::vec2 local = m_Input->GetMousePosition() - m_Context->ViewportPosition;

		if (local.x < 0.0f || local.y < 0.0f || local.x > m_Context->ViewportSize.x || local.y > m_Context->ViewportSize.y)
		{
			return;
		}

		// The renderer flips the viewport height, so screen-top maps to NDC +Y
		const glm::vec2 ndc(
			(local.x / m_Context->ViewportSize.x) * 2.0f - 1.0f,
			1.0f - (local.y / m_Context->ViewportSize.y) * 2.0f);

		const glm::mat4 inverseViewProjection = glm::inverse(m_Camera->GetProjection() * m_Camera->GetView());

		const glm::vec4 nearPoint = inverseViewProjection * glm::vec4(ndc.x, ndc.y, 0.0f, 1.0f); // zero-to-one depth
		const glm::vec4 farPoint = inverseViewProjection * glm::vec4(ndc.x, ndc.y, 1.0f, 1.0f);

		const glm::vec3 origin = glm::vec3(nearPoint) / nearPoint.w;
		const glm::vec3 direction = glm::normalize((glm::vec3(farPoint) / farPoint.w) - origin);

		const Ignition::RaycastHit hit = physics->Raycast(origin, direction);

		if (!hit.Hit)
		{
			m_Context->Selection = {};

			return;
		}

		for (Ignition::Entity entity : m_Context->Scene->GetEntities())
		{
			if (entity.GetID() == hit.EntityID)
			{
				m_Context->Selection = entity;

				break;
			}
		}
	}

	void EditorLayer::OnPlay()
	{
		if (m_Context->Play != PlayState::Edit)
		{
			return;
		}

		// The serializer is the snapshot format: it can never drift from what Save/Load produce
		const Ignition::SceneSerializer serializer(m_Context->Scene, m_Assets);
		m_Snapshot = serializer.SaveToString();

		if (m_Snapshot.empty())
		{
			IG_APP_ERROR("Could not snapshot the scene, staying in edit mode");

			return;
		}

		m_RuntimePhysics = std::make_unique<Ignition::PhysicsWorld>(m_Context->Scene, m_Assets);
		m_RuntimePhysics->SetDebugVisualizationEnabled(m_Context->DrawPhysXVisualization);

		m_Context->Play = PlayState::Playing;

		IG_APP_INFO("Play: {} physics bodies instantiated", m_RuntimePhysics->GetBodyCount());
	}

	void EditorLayer::OnStop()
	{
		if (m_Context->Play == PlayState::Edit)
		{
			return;
		}

		const uint32_t selectionID = m_Context->Selection.IsValid() ? m_Context->Selection.GetID() : 0xFFFFFFFF;

		m_RuntimePhysics.reset();
		m_Context->Selection = {};
		m_Context->Play = PlayState::Edit;
		m_Context->StepRequested = false;

		const Ignition::SceneSerializer serializer(m_Context->Scene, m_Assets);

		if (!serializer.LoadFromString(m_Snapshot))
		{
			IG_APP_ERROR("Snapshot restore failed, the scene is left as the simulation ended it");
		}

		m_Snapshot.clear();
		m_Context->PhysicsSceneDirty = true;

		for (Ignition::Entity entity : m_Context->Scene->GetEntities())
		{
			if (entity.GetID() == selectionID)
			{
				m_Context->Selection = entity;

				break;
			}
		}
	}

	void EditorLayer::SubmitColliderGizmos()
	{
		if (Ignition::PhysicsWorld* physics = GetActivePhysicsWorld())
		{
			physics->SubmitDebugLines();
		}

		if (!m_Context->DrawColliders)
		{
			return;
		}

		for (Ignition::Entity entity : m_Context->Scene->GetEntities())
		{
			const Ignition::TransformComponent& transform = entity.GetTransform();
			const float scale = UniformScale(transform.Scale);

			if (const Ignition::BoxColliderComponent* collider = entity.GetBoxCollider())
			{
				Ignition::DebugDraw::Box(ColliderTransform(transform, collider->Offset), collider->HalfExtents * transform.Scale, ColliderColor);
			}

			if (const Ignition::SphereColliderComponent* collider = entity.GetSphereCollider())
			{
				Ignition::DebugDraw::Sphere(ColliderTransform(transform, collider->Offset), collider->Radius * scale, ColliderColor);
			}

			if (const Ignition::CapsuleColliderComponent* collider = entity.GetCapsuleCollider())
			{
				Ignition::DebugDraw::Capsule(ColliderTransform(transform, collider->Offset), collider->Radius * scale, collider->HalfHeight * scale, ColliderColor);
			}
		}

		if (m_Context->Selection.IsValid())
		{
			Ignition::DebugDraw::Axes(ColliderTransform(m_Context->Selection.GetTransform(), glm::vec3(0.0f)), 0.75f);
		}
	}

	void EditorLayer::OnRender()
	{
		if (!Ignition::UI::IsFrameActive())
		{
			return;
		}

		SubmitColliderGizmos();

		Ignition::UI::BeginGizmoFrame();

		const unsigned int dockspaceID = Ignition::UI::DockSpaceOverMainViewport();

		if (!m_DockLayoutInitialized)
		{
			m_DockLayoutInitialized = true;
			Ignition::UI::BuildDefaultDockLayout(dockspaceID, "Hierarchy", "Inspector", "Stats", "Viewport");
		}

		DrawMenuBar();
		DrawPathPrompt();
		DrawToolbarPanel();
		DrawViewportPanel();
		DrawHierarchyPanel();
		DrawInspectorPanel();
		DrawStatsPanel();
	}

	void EditorLayer::DrawToolbarPanel()
	{
		if (Ignition::UI::BeginWindow("Toolbar"))
		{
			Ignition::UI::BeginDisabled(m_Context->Play == PlayState::Playing);

			if (Ignition::UI::Button(m_Context->Play == PlayState::Paused ? "Resume" : "Play"))
			{
				if (m_Context->Play == PlayState::Paused)
				{
					m_Context->Play = PlayState::Playing;
				}
				else
				{
					OnPlay();
				}
			}

			Ignition::UI::EndDisabled();
			Ignition::UI::SameLine();

			Ignition::UI::BeginDisabled(m_Context->Play != PlayState::Playing);

			if (Ignition::UI::Button("Pause"))
			{
				m_Context->Play = PlayState::Paused;
			}

			Ignition::UI::EndDisabled();
			Ignition::UI::SameLine();

			Ignition::UI::BeginDisabled(m_Context->Play != PlayState::Paused);

			if (Ignition::UI::Button("Step"))
			{
				m_Context->StepRequested = true;
			}

			Ignition::UI::EndDisabled();
			Ignition::UI::SameLine();

			Ignition::UI::BeginDisabled(m_Context->Play == PlayState::Edit);

			if (Ignition::UI::Button("Stop"))
			{
				OnStop();
			}

			Ignition::UI::EndDisabled();

			Ignition::UI::SameLine();
			Ignition::UI::Separator();

			Ignition::UI::Checkbox("Colliders", &m_Context->DrawColliders);
			Ignition::UI::SameLine();

			if (Ignition::UI::Checkbox("PhysX Visualization", &m_Context->DrawPhysXVisualization))
			{
				if (Ignition::PhysicsWorld* physics = GetActivePhysicsWorld())
				{
					physics->SetDebugVisualizationEnabled(m_Context->DrawPhysXVisualization);
				}
			}
		}

		Ignition::UI::EndWindow();
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

		// Play mode is never ambiguous: the viewport frame goes orange while the simulation owns the scene
		const bool playing = m_Context->Play != PlayState::Edit;
		Ignition::UI::PushWindowBorder(playing ? 3.0f : 0.0f, PlayBorderColor);

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
		Ignition::UI::PopWindowBorder();
		Ignition::UI::PopStyleVariable();
	}

	void EditorLayer::DrawGizmo()
	{
		m_Context->GizmoUsing = false;

		if (!m_Context->Selection.IsValid() || m_Context->ViewportSize.x <= 0.0f || m_Context->ViewportSize.y <= 0.0f)
		{
			return;
		}

		Ignition::UI::SetGizmoViewportRect(m_Context->ViewportPosition, m_Context->ViewportSize);

		Ignition::TransformComponent& transform = m_Context->Selection.GetTransform();

		// Hold Ctrl to snap: 0.5 m translate, 15 degrees rotate
		const bool snapping = m_Input->IsKeyDown(Ignition::ScanCode::LCTRL);
		const float snap = snapping ? (m_GizmoOperation == Ignition::UI::GizmoOperation::Rotate ? 15.0f : 0.5f) : 0.0f;
		const Ignition::UI::GizmoMode mode = m_GizmoWorldSpace ? Ignition::UI::GizmoMode::World : Ignition::UI::GizmoMode::Local;

		const bool manipulated = Ignition::UI::TransformGizmo(m_Camera->GetView(), m_Camera->GetProjection(), m_GizmoOperation, mode, transform.Position, transform.Rotation, transform.Scale, snap);

		// Suppress camera input while the gizmo is hot
		m_Context->GizmoUsing = Ignition::UI::IsGizmoInUse() || Ignition::UI::IsGizmoHovered();

		if (!manipulated)
		{
			return;
		}

		if (m_RuntimePhysics)
		{
			m_RuntimePhysics->PushTransform(m_Context->Selection);
		}
		else
		{
			m_Context->PhysicsSceneDirty = true;
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
						m_Context->PhysicsSceneDirty = true;
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
				m_Context->PhysicsSceneDirty = true;
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

				bool transformEdited = Ignition::UI::DragFloat3("Position", transform.Position, 0.01f);
				const bool rotationCacheStale = m_RotationEulerEntity != entity.GetID() || glm::abs(glm::dot(glm::quat(glm::radians(m_RotationEuler)), transform.Rotation)) < 0.9999f;

				if (rotationCacheStale)
				{
					m_RotationEuler = glm::degrees(transform.GetEulerAngles());
					m_RotationEulerEntity = entity.GetID();
				}

				if (Ignition::UI::DragFloat3("Rotation", m_RotationEuler, 0.5f))
				{
					transform.SetEulerAngles(glm::radians(m_RotationEuler));
					transformEdited = true;
				}

				transformEdited |= Ignition::UI::DragFloat3("Scale", transform.Scale, 0.01f);

				if (transformEdited)
				{
					if (m_RuntimePhysics)
					{
						m_RuntimePhysics->PushTransform(entity);
					}
					else
					{
						m_Context->PhysicsSceneDirty = true;
					}
				}
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

			if (Ignition::RigidBodyComponent* rigidBody = entity.GetRigidBody())
			{
				if (Ignition::UI::CollapsingHeader("Rigid Body"))
				{
					static const char* const bodyTypes[] = { "Static", "Kinematic", "Dynamic" };
					int bodyType = static_cast<int>(rigidBody->Type);

					if (Ignition::UI::Combo("Type", &bodyType, bodyTypes, 3))
					{
						rigidBody->Type = static_cast<Ignition::RigidBodyType>(bodyType);
						m_Context->PhysicsSceneDirty = true;
					}

					Ignition::UI::DragFloat("Mass (kg)", &rigidBody->Mass, 0.05f, 0.001f, 100000.0f);
					Ignition::UI::DragFloat("Linear Damping", &rigidBody->LinearDamping, 0.005f, 0.0f, 10.0f);
					Ignition::UI::DragFloat("Angular Damping", &rigidBody->AngularDamping, 0.005f, 0.0f, 10.0f);
					Ignition::UI::Checkbox("Use Gravity", &rigidBody->UseGravity);
				}
			}

			if (Ignition::BoxColliderComponent* collider = entity.GetBoxCollider())
			{
				if (Ignition::UI::CollapsingHeader("Box Collider"))
				{
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat3("Half Extents", collider->HalfExtents, 0.01f);
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat3("Offset##Box", collider->Offset, 0.01f);

					if (Ignition::UI::SmallButton("Remove##Box"))
					{
						entity.RemoveBoxCollider();
						m_Context->PhysicsSceneDirty = true;
					}
				}
			}

			if (Ignition::SphereColliderComponent* collider = entity.GetSphereCollider())
			{
				if (Ignition::UI::CollapsingHeader("Sphere Collider"))
				{
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat("Radius##Sphere", &collider->Radius, 0.01f, 0.001f, 1000.0f);
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat3("Offset##Sphere", collider->Offset, 0.01f);

					if (Ignition::UI::SmallButton("Remove##Sphere"))
					{
						entity.RemoveSphereCollider();
						m_Context->PhysicsSceneDirty = true;
					}
				}
			}

			if (Ignition::CapsuleColliderComponent* collider = entity.GetCapsuleCollider())
			{
				if (Ignition::UI::CollapsingHeader("Capsule Collider"))
				{
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat("Radius##Capsule", &collider->Radius, 0.01f, 0.001f, 1000.0f);
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat("Half Height", &collider->HalfHeight, 0.01f, 0.001f, 1000.0f);
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat3("Offset##Capsule", collider->Offset, 0.01f);

					if (Ignition::UI::SmallButton("Remove##Capsule"))
					{
						entity.RemoveCapsuleCollider();
						m_Context->PhysicsSceneDirty = true;
					}
				}
			}

			if (Ignition::MeshColliderComponent* collider = entity.GetMeshCollider())
			{
				if (Ignition::UI::CollapsingHeader("Mesh Collider"))
				{
					Ignition::UI::LabelText("Mesh", collider->MeshAsset.empty() ? "<unreferenced>" : collider->MeshAsset.c_str());

					if (Ignition::UI::Checkbox("Convex", &collider->Convex))
					{
						m_Context->PhysicsSceneDirty = true;
					}

					Ignition::UI::TextDisabled("Triangle meshes are static-only; dynamic bodies always cook convex");

					if (Ignition::UI::SmallButton("Remove##Mesh"))
					{
						entity.RemoveMeshCollider();
						m_Context->PhysicsSceneDirty = true;
					}
				}
			}

			if (Ignition::PhysicsMaterialComponent* physicsMaterial = entity.GetPhysicsMaterial())
			{
				if (Ignition::UI::CollapsingHeader("Physics Material"))
				{
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat("Static Friction", &physicsMaterial->StaticFriction, 0.01f, 0.0f, 2.0f);
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat("Dynamic Friction", &physicsMaterial->DynamicFriction, 0.01f, 0.0f, 2.0f);
					m_Context->PhysicsSceneDirty |= Ignition::UI::DragFloat("Restitution", &physicsMaterial->Restitution, 0.01f, 0.0f, 1.0f);
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

				if (!entity.GetRigidBody() && Ignition::UI::MenuItem("Rigid Body"))
				{
					entity.AddRigidBody();
					m_Context->PhysicsSceneDirty = true;
				}

				if (!entity.GetBoxCollider() && Ignition::UI::MenuItem("Box Collider"))
				{
					entity.AddBoxCollider();
					m_Context->PhysicsSceneDirty = true;
				}

				if (!entity.GetSphereCollider() && Ignition::UI::MenuItem("Sphere Collider"))
				{
					entity.AddSphereCollider();
					m_Context->PhysicsSceneDirty = true;
				}

				if (!entity.GetCapsuleCollider() && Ignition::UI::MenuItem("Capsule Collider"))
				{
					entity.AddCapsuleCollider();
					m_Context->PhysicsSceneDirty = true;
				}

				if (!entity.GetMeshCollider() && Ignition::UI::MenuItem("Mesh Collider"))
				{
					Ignition::MeshColliderComponent collider;

					if (const Ignition::MeshRendererComponent* meshRenderer = entity.GetMeshRenderer())
					{
						collider.MeshAsset = meshRenderer->MeshAsset;
					}

					entity.AddMeshCollider(collider);
					m_Context->PhysicsSceneDirty = true;
				}

				if (!entity.GetPhysicsMaterial() && Ignition::UI::MenuItem("Physics Material"))
				{
					entity.AddPhysicsMaterial();
					m_Context->PhysicsSceneDirty = true;
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

			const Ignition::PhysicsWorld* physics = GetActivePhysicsWorld();

			Ignition::UI::Text("Physics: {} ({} bodies @ {:.0f} Hz)", m_Context->Play == PlayState::Edit ? "edit" : (m_Context->Play == PlayState::Playing ? "playing" : "paused"), physics ? physics->GetBodyCount() : 0, 1.0f / Ignition::Time::GetFixedTimeStep());

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
		OnStop();

		m_Context->Selection = {};

		for (Ignition::Entity entity : m_Context->Scene->GetEntities())
		{
			m_Context->Scene->DestroyEntity(entity);
		}

		m_Context->ScenePath.clear();
		m_Context->PhysicsSceneDirty = true;
	}

	void EditorLayer::OpenScene(const std::string& filepath)
	{
		OnStop();

		m_Context->Selection = {};
		m_Context->PhysicsSceneDirty = true;

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