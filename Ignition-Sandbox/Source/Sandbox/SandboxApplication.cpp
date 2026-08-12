#include "Ignition/Core/Application.h"
#include "Ignition/Core/EntryPoint.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Input/ActionMap.h"
#include "Ignition/Input/Input.h"
#include "Ignition/Window/Window.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Camera.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Texture.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Renderer/Vertex.h"
#include "Ignition/Core/Time.h"

#include "Sandbox/CameraController.h"
#include "Sandbox/SandboxLayer.h"

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <utility>

namespace Sandbox
{
	class SandboxApplication : public Ignition::Application
	{
	public:
		SandboxApplication() : Ignition::Application(MakeSpecification())
		{

		}

	protected:
		void OnInitialize() override
		{
			IG_APP_INFO("------- INITIALIZING SANDBOX -------");

			GetRenderer()->SetClearColor(0.01f, 0.01f, 0.01f);

			m_Actions = std::make_unique<Ignition::ActionMap>(GetInput());

			m_Actions->AddButton("Jump").Bind(Ignition::ScanCode::SPACE).Bind(Ignition::GamepadButton::SOUTH);
			m_Actions->AddAxis2D("Move").BindKeys(Ignition::ScanCode::W, Ignition::ScanCode::S, Ignition::ScanCode::A, Ignition::ScanCode::D).BindStick(Ignition::GamepadStick::Left);

			const std::vector<Ignition::Vertex> vertices = {
				{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 1.0f, 0.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
			};

			const std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0, 2, 1, 0, 0, 3, 2 };

			m_QuadMesh = GetRenderer()->CreateMesh(vertices, indices);

			m_TestTexture = GetRenderer()->CreateTexture("Assets/Test.png");
			m_QuadMaterial.Albedo = m_TestTexture;

			int pixelWidth = 0;
			int pixelHeight = 0;
			GetWindow()->GetPixelSize(pixelWidth, pixelHeight);

			const float aspectRatio = pixelHeight > 0 ? static_cast<float>(pixelWidth) / static_cast<float>(pixelHeight) : 16.0f / 9.0f;

			m_Camera.SetPerspective(glm::radians(60.0f), aspectRatio, 0.1f, 100.0f);

			m_CameraController = std::make_unique<CameraController>(GetInput(), m_Actions.get());

			PushLayer(std::make_unique<SandboxLayer>(GetRenderer(), &m_QuadMaterial));

			IG_APP_INFO("------- SANDBOX INITIALIZED -------");
		}

		void OnUpdate(float deltaTime) override
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

			const glm::vec2 move = m_Actions->GetAxis2D("Move");

			if (move.x != 0.0f || move.y != 0.0f)
			{
				IG_APP_TRACE("Move: ({}, {})", move.x, move.y);
			}
		}

		void OnFixedUpdate(float fixedTimeStep) override
		{
			(void)fixedTimeStep;
			//IG_APP_TRACE("FixedTimestep: {}", fixedTimeStep);
		}

		void OnRender() override
		{
			const float angle = static_cast<float>(Ignition::Time::GetElapsedTime()) * glm::radians(90.0f);
			const glm::mat4 spinning = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 0.0f, 1.0f));
			const glm::mat4 orbiting = glm::rotate(glm::mat4(1.0f), -angle, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, -0.5f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.35f));

			GetRenderer()->BeginScene(m_Camera);
			GetRenderer()->Submit(m_QuadMesh, m_QuadMaterial, spinning);
			GetRenderer()->Submit(m_QuadMesh, orbiting);
			GetRenderer()->EndScene();
		}

		void OnEvent(Ignition::Event& event) override
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

			dispatcher.Dispatch<Ignition::KeyPressedEvent>([](Ignition::KeyPressedEvent& keyEvent)
			{
				IG_APP_TRACE("Key pressed: {} (scancode {})", std::to_underlying(keyEvent.GetKeyCode()), std::to_underlying(keyEvent.GetScanCode()));

				return false;
			});
		}

		void OnShutdown() override
		{
			IG_APP_INFO("------- SHUTTING DOWN SANDBOX -------");

			m_QuadMaterial.Albedo.reset();
			m_TestTexture.reset();
			m_QuadMesh.reset();

			IG_APP_INFO("------- SANDBOX SHUTDOWN COMPLETE -------");
		}

	private:
		static Ignition::ApplicationSpecification MakeSpecification()
		{
			Ignition::ApplicationSpecification specification{};
			specification.Title = "Sandbox";
			specification.Width = 1920;
			specification.Height = 1080;

			return specification;
		}

		std::unique_ptr<Ignition::ActionMap> m_Actions;
		std::unique_ptr<CameraController> m_CameraController;
		std::shared_ptr<Ignition::Mesh> m_QuadMesh;
		std::shared_ptr<Ignition::Texture> m_TestTexture;
		Ignition::Material m_QuadMaterial;
		Ignition::Camera m_Camera;
	};
}

namespace Ignition
{
	std::unique_ptr<Application> CreateApplication(int argc, char** argv)
	{
		(void)argc;
		(void)argv;

		return std::make_unique<Sandbox::SandboxApplication>();
	}
}