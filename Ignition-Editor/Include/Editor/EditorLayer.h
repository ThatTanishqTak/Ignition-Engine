#pragma once

#include "Editor/EditorContext.h"

#include <Ignition/Core/Layer.h>

#include <string>
#include <vector>

namespace Ignition
{
	class AssetRegistry;
	class Camera;
	class Input;
	class Renderer;
}

namespace Editor
{
	class EditorCameraController;

	class EditorLayer final : public Ignition::Layer
	{
	public:
		EditorLayer(EditorContext* context, Ignition::Camera* camera, EditorCameraController* cameraController, Ignition::AssetRegistry* assets, Ignition::Renderer* renderer, Ignition::Input* input);

		void OnAttach() override;
		void OnUpdate(float deltaTime) override;
		void OnRender() override;

	private:
		void DrawMenuBar();
		void DrawViewportPanel();
		void DrawGizmo();
		void DrawHierarchyPanel();
		void DrawInspectorPanel();
		void DrawStatsPanel();

		void NewScene();
		void OpenScene(const std::string& filepath);
		void SaveScene(const std::string& filepath);

	private:
		EditorContext* m_Context = nullptr;
		Ignition::Camera* m_Camera = nullptr;
		EditorCameraController* m_CameraController = nullptr;
		Ignition::AssetRegistry* m_Assets = nullptr;
		Ignition::Renderer* m_Renderer = nullptr;
		Ignition::Input* m_Input = nullptr;

		int m_GizmoOperation = 0; // ImGuizmo::OPERATION, stored as int to keep this header ImGuizmo-free
		bool m_GizmoWorldSpace = true;
		bool m_DockLayoutInitialized = false;

		std::vector<float> m_FrameTimeHistory;
	};
}