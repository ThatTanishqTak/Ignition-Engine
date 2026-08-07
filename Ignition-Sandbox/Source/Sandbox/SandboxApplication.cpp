#include "Ignition/Core/Application.h"
#include "Ignition/Core/EntryPoint.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"

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

			IG_APP_INFO("------- SANDBOX INITIALIZED -------");
		}

		void OnUpdate(float deltaTime) override
		{
			(void)deltaTime;
			//IG_APP_TRACE("DeltaTime: {}", deltaTime);
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