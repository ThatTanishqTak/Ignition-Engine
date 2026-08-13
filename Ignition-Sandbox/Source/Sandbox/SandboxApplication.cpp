#include "Sandbox/SandboxApplication.h"

#include "Sandbox/CameraController.h"
#include "Sandbox/ControlPanelLayer.h"

#include <Ignition/Core/Log.h>
#include <Ignition/Core/Time.h>
#include <Ignition/Events/Event.h>
#include <Ignition/Events/KeyEvent.h>
#include <Ignition/Events/WindowEvent.h>
#include <Ignition/Input/Input.h>
#include <Ignition/Renderer/Renderer.h>
#include <Ignition/Renderer/Material.h>
#include <Ignition/Renderer/Texture.h>
#include <Ignition/Renderer/Vertex.h>
#include <Ignition/Scene/Components.h>
#include <Ignition/Scene/ModelImporter.h>
#include <Ignition/Window/Window.h>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/trigonometric.hpp>

#include <string>
#include <utility>
#include <vector>

namespace Sandbox
{
	SandboxApplication::SandboxApplication() : Ignition::Application(MakeSpecification())
	{

	}

	SandboxApplication::~SandboxApplication() = default;

	void SandboxApplication::OnInitialize()
	{
		IG_APP_INFO("------- INITIALIZING SANDBOX -------");

		GetRenderer()->SetClearColor(0.01f, 0.01f, 0.01f);

		SetupInput();
		SetupCamera();
		BuildDemoScene();

		PushLayer(std::make_unique<ControlPanelLayer>(GetRenderer(), m_Scene.get()));

		IG_APP_INFO("------- SANDBOX INITIALIZED -------");
	}

	void SandboxApplication::OnUpdate(float deltaTime)
	{
		m_CameraController->Update(m_Camera, deltaTime, GetRenderer()->WantCaptureMouse(), GetRenderer()->WantCaptureKeyboard());

		if (!GetRenderer()->WantCaptureKeyboard() && m_Actions->IsPressed("Jump"))
		{
			IG_APP_INFO("Jump");

			for (Ignition::GamepadID gamepadID : GetInput()->GetConnectedGamepads())
			{
				GetInput()->SetGamepadRumble(gamepadID, 0.5f, 0.5f, 200);
			}
		}

		if (m_SpinningEntity.IsValid())
		{
			m_SpinningEntity.GetTransform().Rotation.z = static_cast<float>(Ignition::Time::GetElapsedTime()) * glm::radians(90.0f);
		}
	}

	void SandboxApplication::OnRender()
	{
		m_Scene->OnRender(*GetRenderer(), m_Camera);
	}

	void SandboxApplication::OnEvent(Ignition::Event& event)
	{
		Ignition::EventDispatcher dispatcher(event);

		dispatcher.Dispatch<Ignition::WindowResizeEvent>([this](Ignition::WindowResizeEvent& resizeEvent)
		{
			if (resizeEvent.GetPixelHeight() > 0)
			{
				m_Camera.SetAspectRatio(static_cast<float>(resizeEvent.GetPixelWidth()) / static_cast<float>(resizeEvent.GetPixelHeight()));
			}

			return false;
		});
	}

	void SandboxApplication::OnShutdown()
	{
		IG_APP_INFO("------- SHUTTING DOWN SANDBOX -------");

		m_Scene.reset();
		m_QuadMesh.reset();

		IG_APP_INFO("------- SANDBOX SHUTDOWN COMPLETE -------");
	}

	void SandboxApplication::SetupInput()
	{
		m_Actions = std::make_unique<Ignition::ActionMap>(GetInput());

		m_Actions->AddButton("Jump").Bind(Ignition::ScanCode::SPACE).Bind(Ignition::GamepadButton::SOUTH);
		m_Actions->AddAxis2D("Move").BindKeys(Ignition::ScanCode::W, Ignition::ScanCode::S, Ignition::ScanCode::A, Ignition::ScanCode::D).BindStick(Ignition::GamepadStick::Left);
	}

	void SandboxApplication::SetupCamera()
	{
		int pixelWidth = 0;
		int pixelHeight = 0;
		GetWindow()->GetPixelSize(pixelWidth, pixelHeight);

		const float aspectRatio = pixelHeight > 0 ? static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight) : 16.0f / 9.0f;

		m_Camera.SetPerspective(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);

		m_CameraController = std::make_unique<CameraController>(GetInput(), m_Actions.get());
	}

	void SandboxApplication::BuildDemoScene()
	{
		m_Scene = std::make_unique<Ignition::Scene>();

		const std::vector<Ignition::Vertex> vertices = {
			{ { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
			{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
			{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		};

		const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0, 2, 1, 0, 0, 3, 2 };

		m_QuadMesh = GetRenderer()->CreateMesh(vertices, indices);

		const std::shared_ptr<Ignition::Texture> testTexture = GetRenderer()->CreateTexture("Assets/Test.png");

		constexpr int gridSize = 5;
		constexpr float spacing = 0.75f;

		for (int y = 0; y < gridSize; ++y)
		{
			for (int x = 0; x < gridSize; ++x)
			{
				Ignition::Entity entity = m_Scene->CreateEntity("Quad (" + std::to_string(x) + ", " + std::to_string(y) + ")");

				Ignition::TransformComponent& transform = entity.GetTransform();
				transform.Position = { (x - gridSize / 2) * spacing, (y - gridSize / 2) * spacing, 0.0f };
				transform.Scale = glm::vec3(0.6f);

				Ignition::Material material{};
				material.Tint = { x / static_cast<float>(gridSize - 1), y / static_cast<float>(gridSize - 1), 1.0f, 1.0f };

				if (x == gridSize / 2 && y == gridSize / 2)
				{
					material.Albedo = testTexture;
					material.Tint = glm::vec4(1.0f);
				}

				entity.AddMeshRenderer(m_QuadMesh, material);
			}
		}

		m_SpinningEntity = m_Scene->CreateEntity("Spinner");
		m_SpinningEntity.GetTransform().Position = { 0.0f, 0.0f, 0.5f };
		m_SpinningEntity.GetTransform().Scale = glm::vec3(0.35f);
		m_SpinningEntity.AddMeshRenderer(m_QuadMesh, {});

		const std::vector<Ignition::Entity> duckEntities = Ignition::ModelImporter::Import(*m_Scene, *GetRenderer(), "Assets/Duck.glb");

		for (Ignition::Entity entity : duckEntities)
		{
			Ignition::TransformComponent& transform = entity.GetTransform();
			transform.Position = { 2.5f, -1.0f, 0.0f };
			transform.Scale = glm::vec3(1.0f);
		}
	}

	Ignition::ApplicationSpecification SandboxApplication::MakeSpecification()
	{
		Ignition::ApplicationSpecification specification{};
		specification.Title = "Ignition Sandbox";
		specification.Width = 1920;
		specification.Height = 1080;

		return specification;
	}
}