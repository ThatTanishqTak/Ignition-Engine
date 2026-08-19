#pragma once

#include "Editor/EditorContext.h"

#include "Ignition/Core/Layer.h"
#include "Ignition/UI/UI.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <array>

#include "Ignition/Fluid/FluidSolver2D.h"
#include "Ignition/Fluid/FluidSolver3D.h"

namespace Ignition
{
	class AssetRegistry;
	class Camera;
	class Input;
	class PhysicsWorld;
	class Renderer;
	struct TransformComponent;
	struct WindTunnelComponent;
}

namespace Editor
{
	class EditorCameraController;

	class EditorLayer final : public Ignition::Layer
	{
	public:
		EditorLayer(EditorContext* context, Ignition::Camera* camera, Ignition::AssetRegistry* assets, Ignition::Renderer* renderer, Ignition::Input* input);

		void OnAttach() override;
		void OnUpdate(float deltaTime) override;
		void OnFixedUpdate(float fixedTimeStep) override;
		void OnRender() override;

	private:
		enum class PathPrompt
		{
			None = 0,
			Open,
			SaveAs
		};

		void DrawMenuBar();
		void DrawToolbarPanel();
		void DrawViewportPanel();
		void DrawGizmo();
		void DrawHierarchyPanel();
		void DrawInspectorPanel();
		void DrawStatsPanel();
		void DrawFluidLabPanel();
		void DrawAeroPanel();
		void DrawPathPrompt();
		void PromptForPath(PathPrompt prompt);

		void UpdateFluidLab();
		void RecordFluidHistory();
		void ClearFluidHistory();

		void UpdateWindTunnel(float deltaTime);
		void GatherAeroBodies();
		void SubmitTunnelGizmos();
		void RecordAeroHistory();
		void ClearAeroHistory();
		Ignition::FluidSolver3DSettings BuildTunnelSettings(const Ignition::TransformComponent& transform, const Ignition::WindTunnelComponent& tunnel) const;

		void NewScene();
		void OpenScene(const std::string& filepath);
		void SaveScene(const std::string& filepath);

		void OnPlay();
		void OnStop();

		void SubmitColliderGizmos();
		void RefreshEditorPhysics();
		void PickEntityUnderCursor();

		Ignition::PhysicsWorld* GetActivePhysicsWorld() const;

	private:
		EditorContext* m_Context = nullptr;
		Ignition::Camera* m_Camera = nullptr;
		Ignition::AssetRegistry* m_Assets = nullptr;
		Ignition::Renderer* m_Renderer = nullptr;
		Ignition::Input* m_Input = nullptr;

		std::unique_ptr<Ignition::PhysicsWorld> m_EditorPhysics;  // always alive, static shapes, raycast picking
		std::unique_ptr<Ignition::PhysicsWorld> m_RuntimePhysics; // built on Play, destroyed on Stop
		std::string m_Snapshot;

		Ignition::UI::GizmoOperation m_GizmoOperation = Ignition::UI::GizmoOperation::Translate;
		bool m_GizmoWorldSpace = true;

		// Inspector-only Euler cache, in degrees
		glm::vec3 m_RotationEuler{ 0.0f };
		uint32_t m_RotationEulerEntity = UINT32_MAX;
		bool m_DockLayoutInitialized = false;

		PathPrompt m_PathPrompt = PathPrompt::None;
		bool m_PathPromptRequested = false;
		std::array<char, 260> m_PathBuffer{};

		std::vector<float> m_FrameTimeHistory;

		// Fluid Lab - the 2D rung of the validation ladder
		std::unique_ptr<Ignition::FluidSolver2D> m_Fluid;
		Ignition::FluidSolver2DSettings m_FluidSettings;
		bool m_FluidLabOpen = true;
		bool m_FluidRunning = false;
		bool m_FluidStepRequested = false;
		int m_FluidStepsPerFrame = 8;
		int m_FluidFieldIndex = 1;
		int m_FluidResolutionIndex = 1;
		float m_FluidColorScale = 1.0f;

		std::vector<float> m_DragHistory;
		std::vector<float> m_LiftHistory;
		float m_PreviousLift = 0.0f;
		float m_PreviousCrossingTime = -1.0f;
		float m_StrouhalNumber = 0.0f;

		// Wind tunnel - the 3D rung. Created lazily, because the lattice is hundreds of megabytes and most scenes have no tunnel in them
		std::unique_ptr<Ignition::FluidSolver3D> m_Tunnel;
		Ignition::FluidSolver3DSettings m_TunnelSettings;
		Ignition::Entity m_TunnelEntity; // the entity carrying the component, so the Aero panel can edit it

		// Voxelizer input, debounced: re-voxelization is heavy, so a gizmo drag re-voxelizes on release rather than every frame
		std::vector<Ignition::FluidBody> m_AeroBodies;
		std::vector<Ignition::FluidBody> m_PendingAeroBodies;
		float m_AeroBodyDebounce = 0.0f;
		bool m_AeroPanelOpen = true;
		bool m_TunnelRunning = false;
		bool m_TunnelStepRequested = false;
		bool m_AeroAveraged = true;
		int m_TunnelStepsPerFrame = 2;

		std::vector<float> m_TunnelDragHistory;
		std::vector<float> m_TunnelDownforceHistory;
	};
}