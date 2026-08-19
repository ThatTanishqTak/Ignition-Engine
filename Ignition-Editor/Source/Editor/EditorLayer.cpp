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
#include <numeric>

namespace Editor
{
	namespace
	{
		constexpr size_t FrameHistorySize = 240;
		constexpr const char* DefaultScenePath = "Assets/Scenes/Scene.yaml";

		// Long enough that an inspector drag settles before the lattice is rebuilt, short enough to feel immediate
		constexpr float AeroBodyDebounceSeconds = 0.25f;

		void PushHistory(std::vector<float>& history, float value)
		{
			if (history.size() >= FrameHistorySize)
			{
				history.erase(history.begin());
			}

			history.push_back(value);
		}

		float HistoryMinimum(const std::vector<float>& history, float fallback)
		{
			return history.empty() ? fallback : *std::min_element(history.begin(), history.end());
		}

		float HistoryMaximum(const std::vector<float>& history, float fallback)
		{
			return history.empty() ? fallback : *std::max_element(history.begin(), history.end());
		}

		// Instantaneous LBM forces oscillate because the flow does; averaging is a reading choice, not noise reduction
		float HistoryAverage(const std::vector<float>& history, float fallback)
		{
			return history.empty() ? fallback : std::accumulate(history.begin(), history.end(), 0.0f) / static_cast<float>(history.size());
		}

		struct TunnelResolution
		{
			const char* Label;
			uint32_t X;
			uint32_t Y;
			uint32_t Z;
		};

		// Long in Z: the wake needs room downstream, the same reason the 2D channel is wide
		constexpr std::array<TunnelResolution, 4> TunnelResolutions = { {
			{ "64 x 32 x 128",    64,  32, 128 },
			{ "128 x 64 x 256",  128,  64, 256 },
			{ "128 x 128 x 256", 128, 128, 256 },
			{ "256 x 128 x 512", 256, 128, 512 },
		} };

		// D3Q19 ping-pong plus mask, macroscopic fields and per-cell force - Appendix B's budget, measured off the actual layout rather than quoted
		float LatticeMegabytes(const glm::uvec3& resolution)
		{
			const double cells = static_cast<double>(resolution.x) * resolution.y * resolution.z;
			const double bytes = cells * (19.0 * 2.0 * sizeof(float) + sizeof(uint32_t) + 4.0 * sizeof(float) + 3.0 * sizeof(float));

			return static_cast<float>(bytes / (1024.0 * 1024.0));
		}

		constexpr glm::vec4 ColliderColor{ 0.2f, 0.9f, 0.3f, 1.0f };
		constexpr glm::vec4 PlayBorderColor{ 1.0f, 0.55f, 0.1f, 1.0f };
		constexpr glm::vec4 TunnelColor{ 0.35f, 0.65f, 1.0f, 1.0f };
		constexpr glm::vec4 FlowColor{ 0.95f, 0.85f, 0.2f, 1.0f };

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
		m_TunnelDragHistory.reserve(FrameHistorySize);
		m_TunnelDownforceHistory.reserve(FrameHistorySize);
	}

	void EditorLayer::OnAttach()
	{
		// Edit-mode raycasts need a live query world, so it is created up front and kept alive
		Ignition::PhysicsSettings settings{};
		settings.QueryOnly = true;

		m_EditorPhysics = std::make_unique<Ignition::PhysicsWorld>(m_Context->Scene, m_Assets, settings);
		m_Context->PhysicsSceneDirty = true;

		// The shipped tunnel scene is the starter content; the fallback exists so a clone missing its assets still opens onto something
		const Ignition::SceneSerializer serializer(m_Context->Scene, m_Assets);

		if (serializer.Load(DefaultScenePath))
		{
			m_Context->ScenePath = DefaultScenePath;
		}
		else
		{
			CreateFallbackScene();
		}
	}

	void EditorLayer::CreateFallbackScene()
	{
		Ignition::Entity body = m_Context->Scene->CreateEntity("Test Body");
		body.GetTransform().Position = { 0.0f, 0.5f, 0.0f };

		if (auto mesh = m_Assets->LoadMesh("builtin:cube"))
		{
			auto& meshRenderer = body.AddMeshRenderer(mesh);
			meshRenderer.MeshAsset = "builtin:cube";
		}

		// Static: a wind tunnel model is mounted, not dropped. Play means the wind starts, not that things fall
		Ignition::RigidBodyComponent rigidBody;
		rigidBody.Type = Ignition::RigidBodyType::Static;

		body.AddRigidBody(rigidBody);
		body.AddBoxCollider();

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

		UpdateWindTunnel(deltaTime);

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
		m_RuntimePhysics->SetDebugVisualizationEnabled(m_Context->DrawPhysicsVisualization);

		// Every run starts from still air, the same way it starts from the snapshot
		if (m_Tunnel)
		{
			m_Tunnel->Reset();
			ClearAeroHistory();
		}

		m_Context->Play = PlayState::Playing;

		IG_APP_INFO("Play: wind on, {} physics bodies instantiated", m_RuntimePhysics->GetBodyCount());
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
		m_Context->TunnelStepRequested = false;

		if (m_Tunnel)
		{
			m_Tunnel->Reset();
			ClearAeroHistory();
		}

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
		IG_PROFILE_FUNCTION();

		if (!Ignition::UI::IsFrameActive())
		{
			return;
		}

		SubmitColliderGizmos();
		SubmitTunnelGizmos();

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
		DrawAeroPanel();
	}

	void EditorLayer::GatherAeroBodies()
	{
		m_AeroBodies.clear();

		// Every mesh in the scene is in the wind - the tunnel is the scene, so nothing has to opt in
		uint32_t objectID = 1;

		for (Ignition::Entity entity : m_Context->Scene->GetEntities())
		{
			const Ignition::MeshRendererComponent* meshRenderer = entity.GetMeshRenderer();

			if (!meshRenderer || !meshRenderer->Mesh || meshRenderer->Mesh->GetGeometry().Indices.size() < 3)
			{
				continue;
			}

			Ignition::FluidBody body{};
			body.Geometry = &meshRenderer->Mesh->GetGeometry();
			body.Transform = entity.GetTransform().GetMatrix();
			body.ObjectID = objectID;

			m_AeroBodies.push_back(body);

			// The mask has one byte for the id, so bodies past 255 share the last one rather than wrapping onto another body's
			objectID = glm::min(objectID + 1u, 255u);
		}
	}

	void EditorLayer::UpdateWindTunnel(float deltaTime)
	{
		IG_PROFILE_FUNCTION();

		const Ignition::FluidSolver3DSettings& settings = m_Context->Scene->GetWindTunnel();

		if (!m_Tunnel)
		{
			m_Tunnel = std::make_unique<Ignition::FluidSolver3D>(m_Renderer, settings);
			m_TunnelSettings = settings;

			if (!m_Tunnel->IsValid())
			{
				IG_APP_ERROR("Wind tunnel unavailable: check that Fluid3D.spv built, and the log for allocation or pipeline errors");
			}
		}
		else if (settings != m_TunnelSettings)
		{
			// Resolution changes rebuild inside the facade; everything else rides in on push constants
			m_TunnelSettings = settings;
			m_Tunnel->Configure(settings);
		}

		if (!m_Tunnel->IsValid())
		{
			return;
		}

		// Voxelization rebuilds the mask and resets the flow, so it waits for the edit to settle: never mid-gizmo-drag, and never until the geometry has held still for a moment
		GatherAeroBodies();

		if (m_AeroBodies != m_PendingAeroBodies)
		{
			m_PendingAeroBodies = m_AeroBodies;
			m_AeroBodyDebounce = AeroBodyDebounceSeconds;
		}
		else if (m_AeroBodyDebounce > 0.0f)
		{
			m_AeroBodyDebounce -= deltaTime;
		}

		if (m_AeroBodyDebounce <= 0.0f && !Ignition::UI::IsGizmoInUse())
		{
			m_Tunnel->SetBodies(m_PendingAeroBodies);
		}

		// The wind runs on the editor's transport. The lattice has its own timestep, so it advances per rendered frame rather than per fixed step
		uint32_t steps = 0;

		if (m_Context->Play == PlayState::Playing)
		{
			steps = static_cast<uint32_t>(m_TunnelStepsPerFrame);
		}
		else if (m_Context->Play == PlayState::Paused && m_Context->TunnelStepRequested)
		{
			steps = 1;
		}

		m_Context->TunnelStepRequested = false;

		if (steps == 0)
		{
			return;
		}

		m_Tunnel->Step(steps);

		// Only simulated frames are recorded, so a paused tunnel does not flatten its own moving average
		RecordAeroHistory();
	}

	void EditorLayer::RecordAeroHistory()
	{
		const Ignition::AeroForces forces = m_Tunnel->GetForces();

		PushHistory(m_TunnelDragHistory, forces.Drag);
		PushHistory(m_TunnelDownforceHistory, forces.Downforce);

		IG_PROFILE_PLOT("Tunnel Drag (N)", forces.Drag);
		IG_PROFILE_PLOT("Tunnel Downforce (N)", forces.Downforce);
	}

	void EditorLayer::ClearAeroHistory()
	{
		m_TunnelDragHistory.clear();
		m_TunnelDownforceHistory.clear();
	}

	void EditorLayer::SubmitTunnelGizmos()
	{
		if (!m_Context->DrawTunnelBounds)
		{
			return;
		}

		const Ignition::FluidSolver3DSettings& settings = m_Context->Scene->GetWindTunnel();
		const Ignition::FluidLatticeScaling scaling = Ignition::FluidSolver3D::ComputeScaling(settings);

		// The lattice is what actually gets simulated, so the lattice is what gets drawn - cubic cells make it a little larger than the requested box
		const glm::vec3 extent = glm::vec3(settings.Resolution) * scaling.CellSize;
		const glm::vec3 center(0.0f, 0.5f * extent.y, 0.0f);

		Ignition::DebugDraw::Box(glm::translate(glm::mat4(1.0f), center), extent * 0.5f, TunnelColor);

		// Air flows in -Z, so the arrow leaves the inlet face pointing at the car's nose
		const glm::vec3 inlet = center + glm::vec3(0.0f, 0.0f, 0.5f * extent.z);
		Ignition::DebugDraw::Arrow(inlet, inlet - glm::vec3(0.0f, 0.0f, glm::min(1.0f, 0.25f * extent.z)), FlowColor);
	}

	void EditorLayer::DrawAeroPanel()
	{
		if (!m_AeroPanelOpen)
		{
			return;
		}

		if (Ignition::UI::BeginWindow("Aero", &m_AeroPanelOpen))
		{
			if (!m_Tunnel || !m_Tunnel->IsValid())
			{
				Ignition::UI::TextDisabled("Wind tunnel unavailable - check that Fluid3D.spv built and the log for pipeline errors");

				Ignition::UI::EndWindow();

				return;
			}

			Ignition::FluidSolver3DSettings& tunnel = m_Context->Scene->GetWindTunnel();

			const Ignition::FluidLatticeScaling& scaling = m_Tunnel->GetScaling();
			const Ignition::AeroForces forces = m_Tunnel->GetForces();

			// No transport of its own: the toolbar drives the wind, so this only says what the toolbar is doing
			switch (m_Context->Play)
			{
				case PlayState::Playing: Ignition::UI::Text("Wind on - {} lattice steps per frame", m_TunnelStepsPerFrame); break;
				case PlayState::Paused:  Ignition::UI::Text("Held - Step advances one lattice step"); break;
				default:                 Ignition::UI::TextDisabled("Still air - press Play to start the wind"); break;
			}

			Ignition::UI::SliderInt("Steps / Frame", &m_TunnelStepsPerFrame, 1, 16);
			Ignition::UI::Checkbox("Moving Average", &m_AeroAveraged);

			Ignition::UI::SeparatorText("Geometry");

			const Ignition::FluidVoxelStatus voxels = m_Tunnel->GetVoxelStatus();

			if (voxels.Voxelized)
			{
				Ignition::UI::Text("{} bodies | {} triangles | {} solid cells", voxels.BodyCount, voxels.TriangleCount, voxels.SolidCells);
			}
			else
			{
				Ignition::UI::TextDisabled("Nothing in the tunnel - every mesh in the scene is voxelized, so add one and the wind will find it");
			}

			if (voxels.FloodResidual > 0)
			{
				// The interior fill silently turns unreached pockets into solid, so non-convergence has to be said out loud
				Ignition::UI::TextDisabled("Flood fill did not converge ({} cells still moving) - raise Flood Iterations, or the interior fill has sealed off open air", voxels.FloodResidual);
			}

			if (voxels.RejectedTriangles > 0)
			{
				Ignition::UI::TextDisabled("{} triangles spanned too much of the domain and were skipped - the mesh needs decimating, or the tunnel is too small for it", voxels.RejectedTriangles);
			}

			Ignition::UI::SeparatorText("Force (Newtons)");

			const float drag = m_AeroAveraged ? HistoryAverage(m_TunnelDragHistory, forces.Drag) : forces.Drag;
			const float downforce = m_AeroAveraged ? HistoryAverage(m_TunnelDownforceHistory, forces.Downforce) : forces.Downforce;

			Ignition::UI::Text("Drag      {:>10.2f} N   Cd {:.3f}", drag, forces.DragCoefficient);
			Ignition::UI::Text("Downforce {:>10.2f} N   Cl {:.3f}", downforce, forces.LiftCoefficient);
			Ignition::UI::Text("Side      {:>10.2f} N", forces.SideForce);
			Ignition::UI::Text("Balance   {:>9.1f}% front | frontal area {:.4f} m2", forces.FrontBalance * 100.0f, forces.ReferenceArea);

			if (forces.ReferenceArea <= 0.0f)
			{
				Ignition::UI::TextDisabled("No solid cells in the lattice - the coefficients are undefined until geometry lands inside the domain");
			}

			if (!m_TunnelDragHistory.empty())
			{
				Ignition::UI::PlotLines("##TunnelDrag", m_TunnelDragHistory.data(), static_cast<int>(m_TunnelDragHistory.size()), HistoryMinimum(m_TunnelDragHistory, 0.0f), HistoryMaximum(m_TunnelDragHistory, 1.0f), 60.0f, "Drag (N)");
				Ignition::UI::PlotLines("##TunnelDownforce", m_TunnelDownforceHistory.data(), static_cast<int>(m_TunnelDownforceHistory.size()), HistoryMinimum(m_TunnelDownforceHistory, -1.0f), HistoryMaximum(m_TunnelDownforceHistory, 1.0f), 60.0f, "Downforce (N)");
			}

			Ignition::UI::SeparatorText("Lattice Mapping");

			Ignition::UI::Text("Cell {:.4f} m | dt {:.3e} s | {} cells across the reference length", scaling.CellSize, scaling.TimeStep, static_cast<int>(scaling.ReferenceLengthCells));
			Ignition::UI::Text("tau {:.5f} | u_lattice {:.3f} | Re {:.3e}", scaling.RelaxationTime, scaling.LatticeVelocity, scaling.ReynoldsNumber);
			Ignition::UI::Text("nu {:.3e} requested | {:.3e} m2/s resolved", scaling.KinematicViscosity, scaling.EffectiveViscosity);
			Ignition::UI::Text("Steps {} | simulated {:.4f} s", m_Tunnel->GetStepCount(), m_Tunnel->GetSimulatedTime());

			if (scaling.SubgridLimited)
			{
				Ignition::UI::TextDisabled("Air's viscosity is finer than this lattice resolves - the relaxation floor and Smagorinsky are carrying it, which is what a large-eddy simulation does");
			}

			if (!scaling.Stable)
			{
				Ignition::UI::TextDisabled("Unstable mapping: keep the lattice velocity at or below 0.1 and at least 8 cells across the reference length");
			}

			Ignition::UI::SeparatorText("Domain");

			int resolutionIndex = 0;

			for (size_t preset = 0; preset < TunnelResolutions.size(); ++preset)
			{
				if (tunnel.Resolution.x == TunnelResolutions[preset].X && tunnel.Resolution.y == TunnelResolutions[preset].Y && tunnel.Resolution.z == TunnelResolutions[preset].Z)
				{
					resolutionIndex = static_cast<int>(preset);

					break;
				}
			}

			static const char* const resolutions[] = { TunnelResolutions[0].Label, TunnelResolutions[1].Label, TunnelResolutions[2].Label, TunnelResolutions[3].Label };

			// The only control that reallocates, and the only one worth checking against the dev GPU's VRAM first
			if (Ignition::UI::Combo("Resolution", &resolutionIndex, resolutions, static_cast<int>(TunnelResolutions.size())))
			{
				tunnel.Resolution = { TunnelResolutions[resolutionIndex].X, TunnelResolutions[resolutionIndex].Y, TunnelResolutions[resolutionIndex].Z };
			}

			Ignition::UI::Text("Lattice: {:.0f} MB", LatticeMegabytes(tunnel.Resolution));
			Ignition::UI::DragFloat3("Domain Size (m)", tunnel.DomainSize, 0.05f);
			Ignition::UI::TextDisabled("Cells are cubic, so the lattice is at least this large and sits floor-down on the world origin");

			Ignition::UI::SeparatorText("Flow");

			Ignition::UI::DragFloat("Inlet Speed (m/s)", &tunnel.InletSpeed, 0.5f, 1.0f, 120.0f);
			Ignition::UI::DragFloat("Air Density", &tunnel.AirDensity, 0.001f, 0.1f, 5.0f);
			Ignition::UI::DragFloat("Kinematic Viscosity", &tunnel.KinematicViscosity, 1e-7f, 1e-7f, 1e-3f);
			Ignition::UI::SliderFloat("Lattice Velocity", &tunnel.LatticeVelocity, 0.01f, 0.15f);
			Ignition::UI::SliderFloat("Smagorinsky C", &tunnel.SmagorinskyConstant, 0.0f, 0.3f);
			Ignition::UI::SliderFloat("Minimum tau", &tunnel.MinimumRelaxationTime, 0.5005f, 0.55f);

			Ignition::UI::Checkbox("Rolling Road", &tunnel.RollingRoad);
			Ignition::UI::TextDisabled("A static floor grows a false boundary layer that corrupts underbody flow - not optional for automotive work");

			Ignition::UI::SeparatorText("Reference");

			Ignition::UI::DragFloat("Reference Length (m)", &tunnel.ReferenceLength, 0.01f, 0.01f, 20.0f);
			Ignition::UI::DragFloat("Wheelbase (m)", &tunnel.Wheelbase, 0.01f, 0.1f, 20.0f);
			Ignition::UI::DragFloat3("Reference Point", tunnel.ReferencePoint, 0.01f);

			int floodIterations = static_cast<int>(tunnel.FloodIterations);

			if (Ignition::UI::SliderInt("Flood Iterations", &floodIterations, 1, 32))
			{
				tunnel.FloodIterations = static_cast<uint32_t>(floodIterations);
			}

			Ignition::UI::SeparatorText("Visualization");

			Ignition::UI::Checkbox("Slice Plane", &tunnel.SliceEnabled);

			static const char* const axes[] = { "X (side view)", "Y (top view)", "Z (front view)" };
			int sliceAxis = static_cast<int>(tunnel.SliceAxis);

			if (Ignition::UI::Combo("Axis##Slice", &sliceAxis, axes, 3))
			{
				tunnel.SliceAxis = static_cast<Ignition::FluidSliceAxis>(sliceAxis);
			}

			Ignition::UI::SliderFloat("Position##Slice", &tunnel.SlicePosition, 0.0f, 1.0f);

			static const char* const sliceFields[] = { "Velocity", "Vorticity", "Pressure" };
			int sliceField = static_cast<int>(tunnel.SliceField);

			if (Ignition::UI::Combo("Field##Slice", &sliceField, sliceFields, 3))
			{
				tunnel.SliceField = static_cast<Ignition::FluidField>(sliceField);
			}

			Ignition::UI::SliderFloat("Color Scale##Tunnel", &tunnel.ColorScale, 0.1f, 4.0f);
			Ignition::UI::SliderFloat("Opacity##Slice", &tunnel.SliceOpacity, 0.1f, 1.0f);

			Ignition::UI::Checkbox("Tracer Particles", &tunnel.ParticlesEnabled);

			int particleCount = static_cast<int>(tunnel.ParticleCount);

			if (Ignition::UI::SliderInt("Count##Particles", &particleCount, 1024, 262144))
			{
				tunnel.ParticleCount = static_cast<uint32_t>(particleCount);
			}

			Ignition::UI::Checkbox("Surface Shell", &tunnel.SurfacePressureEnabled);
			Ignition::UI::SameLine();
			Ignition::UI::Checkbox("Show Voxel Mask", &tunnel.VoxelDebugView);

			if (tunnel.SliceEnabled && m_Tunnel->GetSliceTextureID() != 0)
			{
				// The preview shares the in-scene texture; aspect follows the plane the axis selects
				const glm::uvec3 resolution = m_Tunnel->GetSettings().Resolution;

				float sliceAspect = static_cast<float>(resolution.y) / static_cast<float>(resolution.x);

				if (tunnel.SliceAxis == Ignition::FluidSliceAxis::X)
				{
					sliceAspect = static_cast<float>(resolution.y) / static_cast<float>(resolution.z);
				}
				else if (tunnel.SliceAxis == Ignition::FluidSliceAxis::Y)
				{
					sliceAspect = static_cast<float>(resolution.z) / static_cast<float>(resolution.x);
				}

				const float previewWidth = Ignition::UI::GetContentRegionAvailable().x;

				Ignition::UI::Image(m_Tunnel->GetSliceTextureID(), previewWidth, previewWidth * sliceAspect);
			}
		}

		Ignition::UI::EndWindow();
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
				m_Context->TunnelStepRequested = true;
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
			Ignition::UI::Checkbox("Tunnel Bounds", &m_Context->DrawTunnelBounds);
			Ignition::UI::SameLine();

			if (Ignition::UI::Checkbox("Physics Visualization", &m_Context->DrawPhysicsVisualization))
			{
				if (Ignition::PhysicsWorld* physics = GetActivePhysicsWorld())
				{
					physics->SetDebugVisualizationEnabled(m_Context->DrawPhysicsVisualization);
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

		if (Ignition::UI::BeginMenu("View"))
		{
			if (Ignition::UI::MenuItem("Aero"))
			{
				m_AeroPanelOpen = true;
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

			const std::vector<Ignition::PassTiming>& timings = m_Renderer->GetPassTimings();

			if (!timings.empty())
			{
				Ignition::UI::SeparatorText("GPU");

				for (const Ignition::PassTiming& timing : timings)
				{
					Ignition::UI::Text("{}: {:.3f} ms", timing.Name, timing.Milliseconds);
				}
			}

			const bool capturing = Ignition::Profiler::IsConnected();

			Ignition::UI::SeparatorText("Profiler");

			if (capturing)
			{
				Ignition::UI::Text("Tracy: capturing");
			}
			else
			{
				Ignition::UI::TextDisabled("Tracy: waiting for a viewer");
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

		// An empty scene is still a tunnel, just an empty one - the settings go back to defaults with it
		m_Context->Scene->GetWindTunnel() = {};
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