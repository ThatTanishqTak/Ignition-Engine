#include "Ignition/Core/Application.h"
#include "Ignition/Core/EntryPoint.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Input/ActionMap.h"
#include "Ignition/Input/Input.h"
#include "Ignition/Renderer/Renderer.h"

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

			IG_APP_INFO("------- SANDBOX INITIALIZED -------");
		}

		void OnUpdate(float deltaTime) override
		{
			(void)deltaTime;

			if (m_Actions->IsPressed("Jump"))
			{
				IG_APP_INFO("Jump");

				for (Ignition::GamepadID gamepadID : GetInput()->GetConnectedGamepads())
				{
					GetInput()->SetGamepadRumble(gamepadID, 0.5f, 0.5f, 200);
				}
			}

			const Ignition::Float2 move = m_Actions->GetAxis2D("Move");

			if (move.X != 0.0f || move.Y != 0.0f)
			{
				IG_APP_TRACE("Move: ({}, {})", move.X, move.Y);
			}
		}

		void OnFixedUpdate(float fixedTimeStep) override
		{
			(void)fixedTimeStep;
			//IG_APP_TRACE("FixedTimestep: {}", fixedTimeStep);
		}

		void OnEvent(Ignition::Event& event) override
		{
			Ignition::EventDispatcher dispatcher(event);

			dispatcher.Dispatch<Ignition::KeyPressedEvent>([](Ignition::KeyPressedEvent& keyEvent)
			{
				IG_APP_TRACE("Key pressed: {} (scancode {})", std::to_underlying(keyEvent.GetKeyCode()), std::to_underlying(keyEvent.GetScanCode()));

				return false;
			});
		}

		void OnShutdown() override
		{
			IG_APP_INFO("------- SHUTTING DOWN SANDBOX -------");

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