#include "Sandbox/SandboxApplication.h"

#include "Sandbox/CameraController.h"
#include "Sandbox/ControlPanelLayer.h"

#include "Ignition/Ignition.h"

#include <glm/trigonometric.hpp>

#include <memory>

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

		m_Scene = std::make_unique<Ignition::Scene>();

		PushLayer(std::make_unique<ControlPanelLayer>(m_Scene.get()));

		IG_APP_INFO("------- SANDBOX INITIALIZED -------");
	}

	void SandboxApplication::OnUpdate(float deltaTime)
	{
		m_CameraController->Update(m_Camera, deltaTime, GetRenderer()->WantCaptureMouse(), GetRenderer()->WantCaptureKeyboard());
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

		IG_APP_INFO("------- SANDBOX SHUTDOWN COMPLETE -------");
	}

	void SandboxApplication::SetupInput()
	{
		m_Actions = std::make_unique<Ignition::ActionMap>(GetInput());

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

	Ignition::ApplicationSpecification SandboxApplication::MakeSpecification()
	{
		Ignition::ApplicationSpecification specification{};
		specification.Title = "Ignition Sandbox";
		specification.Width = 1920;
		specification.Height = 1080;

		return specification;
	}
}