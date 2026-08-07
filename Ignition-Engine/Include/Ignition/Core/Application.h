#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	struct ApplicationSpecification
	{
		const char* Title = "Ignition";
		int Width = 1280;
		int Height = 720;
	};

	class Engine;
	class Event;

	class Application
	{
	public:
		IGNITION_API Application();
		IGNITION_API explicit Application(const ApplicationSpecification& specification);
		IGNITION_API virtual ~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		IGNITION_API void Initialize();
		IGNITION_API void Shutdown();
		IGNITION_API void Run();

	protected:
		virtual void OnInitialize() {}
		virtual void OnUpdate(float deltaTime) { (void)deltaTime; }
		virtual void OnFixedUpdate(float fixedTimeStep) { (void)fixedTimeStep; }
		virtual void OnEvent(Event& event) { (void)event; }
		virtual void OnShutdown() {}

	private:
		ApplicationSpecification m_Specification;
		std::unique_ptr<Engine> m_Engine;
	};

	std::unique_ptr<Application> CreateApplication(int argc, char** argv);
}