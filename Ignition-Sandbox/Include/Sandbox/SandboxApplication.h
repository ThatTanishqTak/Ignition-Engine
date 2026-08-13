#pragma once

#include <Ignition/Core/Application.h>
#include <Ignition/Input/ActionMap.h>
#include <Ignition/Renderer/Camera.h>
#include <Ignition/Renderer/Mesh.h>
#include <Ignition/Scene/Scene.h>
#include <Ignition/Scene/Entity.h>

#include <memory>

namespace Sandbox
{
	class CameraController;

	class SandboxApplication final : public Ignition::Application
	{
	public:
		SandboxApplication();
		~SandboxApplication() override;

	protected:
		void OnInitialize() override;
		void OnUpdate(float deltaTime) override;
		void OnRender() override;
		void OnEvent(Ignition::Event& event) override;
		void OnShutdown() override;

	private:
		void SetupInput();
		void SetupCamera();
		void BuildDemoScene();

		static Ignition::ApplicationSpecification MakeSpecification();

	private:
		std::unique_ptr<Ignition::ActionMap> m_Actions;
		std::unique_ptr<CameraController> m_CameraController;
		std::unique_ptr<Ignition::Scene> m_Scene;
		std::shared_ptr<Ignition::Mesh> m_QuadMesh;
		Ignition::Entity m_SpinningEntity;
		Ignition::Camera m_Camera;
	};
}