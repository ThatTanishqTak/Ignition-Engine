#pragma once

#include "Editor/EditorContext.h"

#include <Ignition/Core/Layer.h>
#include <Ignition/UI/UI.h>

#include <memory>
#include <string>
#include <vector>
#include <array>

namespace Ignition
{
	class AssetRegistry;
	class Camera;
	class Input;
	class PhysicsWorld;
	class Renderer;
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
		void DrawPathPrompt();
		void PromptForPath(PathPrompt prompt);

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
		bool m_DockLayoutInitialized = false;

		PathPrompt m_PathPrompt = PathPrompt::None;
		bool m_PathPromptRequested = false;
		std::array<char, 260> m_PathBuffer{};

		std::vector<float> m_FrameTimeHistory;
	};
}