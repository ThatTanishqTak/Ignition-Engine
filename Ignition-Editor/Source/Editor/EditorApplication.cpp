#include "Editor/EditorApplication.h"

#include "Editor/EditorCameraController.h"
#include "Editor/EditorLayer.h"

#include "Ignition/Ignition.h"

#include <glm/trigonometric.hpp>

#include <memory>

namespace Editor
{
	EditorApplication::EditorApplication() : Ignition::Application(MakeSpecification())
	{

	}

	EditorApplication::~EditorApplication() = default;

	void EditorApplication::OnInitialize()
	{
		IG_APP_INFO("------- INITIALIZING EDITOR -------");

		GetRenderer()->SetClearColor(0.01f, 0.01f, 0.01f);

		m_Scene = std::make_unique<Ignition::Scene>();
		m_Assets = std::make_unique<Ignition::AssetRegistry>(GetRenderer());
		m_Context.Scene = m_Scene.get();

		m_Camera.SetPerspective(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 1000.0f);
		m_CameraController = std::make_unique<EditorCameraController>(GetInput());

		// Starter content so a fresh session has something selectable
		Ignition::Entity quad = m_Scene->CreateEntity("Quad");

		if (auto mesh = m_Assets->LoadMesh("builtin:quad"))
		{
			auto& meshRenderer = quad.AddMeshRenderer(mesh);
			meshRenderer.MeshAsset = "builtin:quad";
		}

		PushLayer(std::make_unique<EditorLayer>(&m_Context, &m_Camera, m_CameraController.get(), m_Assets.get(), GetRenderer(), GetInput()));

		IG_APP_INFO("------- EDITOR INITIALIZED -------");
	}

	void EditorApplication::OnUpdate(float deltaTime)
	{
		m_CameraController->Update(m_Camera, deltaTime, m_Context);
	}

	void EditorApplication::OnRender()
	{
		m_Scene->OnRender(*GetRenderer(), m_Camera);
	}

	void EditorApplication::OnShutdown()
	{
		IG_APP_INFO("------- SHUTTING DOWN EDITOR -------");

		m_Context.Scene = nullptr;
		m_Context.Selection = {};
		m_Assets.reset();
		m_Scene.reset();

		IG_APP_INFO("------- EDITOR SHUTDOWN COMPLETE -------");
	}

	Ignition::ApplicationSpecification EditorApplication::MakeSpecification()
	{
		Ignition::ApplicationSpecification specification{};
		specification.Title = "Ignition Editor";
		specification.Width = 1920;
		specification.Height = 1080;

		return specification;
	}
}