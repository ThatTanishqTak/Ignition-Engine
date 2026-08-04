#include <Ignition/Core/Application.h>
#include <Ignition/Core/EntryPoint.h>
#include <Ignition/Core/Log.h>

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
			APP_INFO("------- INITIALIZING SANDBOX -------");

			APP_INFO("------- SANDBOX INITIALIZED -------");
		}

		void OnUpdate(float deltaTime) override
		{
			//APP_TRACE("DeltaTime: {}", deltaTime);
		}

		void OnFixedUpdate(float fixedTimeStep) override
		{
			//APP_TRACE("FixedTimestep: {}", fixedTimeStep);
		}

		void OnShutdown() override
		{
			APP_INFO("------- SHUTTING DOWN SANDBOX -------");

			APP_INFO("------- SANDBOX SHUTDOWN COMPLETE -------");
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