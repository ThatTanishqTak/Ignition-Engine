#pragma once

#include "Editor/EditorContext.h"

#include <Ignition/Assets/AssetRegistry.h>
#include <Ignition/Core/Application.h>
#include <Ignition/Renderer/Camera.h>
#include <Ignition/Scene/Scene.h>

#include <memory>

namespace Editor
{
	class EditorCameraController;

	class EditorApplication final : public Ignition::Application
	{
	public:
		EditorApplication();
		~EditorApplication() override;

	protected:
		void OnInitialize() override;
		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnShutdown() override;

	private:
		static Ignition::ApplicationSpecification MakeSpecification();

	private:
		std::unique_ptr<Ignition::Scene> m_Scene;
		std::unique_ptr<Ignition::AssetRegistry> m_Assets;
		std::unique_ptr<EditorCameraController> m_CameraController;
		Ignition::Camera m_Camera;
		EditorContext m_Context;
	};
}