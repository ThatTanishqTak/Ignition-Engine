#include "Ignition/Core/Engine.h"

#include "Ignition/Window/Window.h"
#include "Ignition/Core/Log.h"

namespace Ignition
{
	Engine::Engine() = default;
	Engine::~Engine() = default;

	void Engine::Initialize()
	{
		CORE_INFO("------- INITIALIZING IGNITION -------");

		m_Window = std::make_unique<Window>();
		m_Window->Initialize("Sandbox", 1920, 1080);

		CORE_INFO("------- IGNITION INITIALIZED -------");
	}

	void Engine::Shutdown()
	{
		CORE_INFO("------- IGNITION SHUTTING DOWN -------");

		m_Window->Shutdown();
		m_Window.reset();

		CORE_INFO("------- IGNITION SHUTDOWN COMPLETE -------");
	}

	void Engine::Run()
	{
		while (m_Window->IsOpen())
		{
			m_Window->PollEvents();
		}
	}
}