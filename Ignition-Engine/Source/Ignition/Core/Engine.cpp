#include "Ignition/Core/Engine.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Window/Window.h"

namespace Ignition
{
	Engine::Engine() = default;
	Engine::~Engine() = default;

	void Engine::Initialize(const char* title, int width, int height)
	{
		CORE_INFO("------- INITIALIZING IGNITION -------");

		m_Window = std::make_unique<Window>();
		m_Window->Initialize(title, width, height);

		CORE_INFO("------- IGNITION INITIALIZED -------");
	}

	void Engine::Shutdown()
	{
		CORE_INFO("------- IGNITION SHUTTING DOWN -------");

		if (m_Window)
		{
			m_Window->Shutdown();
			m_Window.reset();
		}

		CORE_INFO("------- IGNITION SHUTDOWN COMPLETE -------");
	}

	void Engine::Update()
	{
		if (m_Window)
		{
			m_Window->PollEvents(m_EventQueue);
		}
	}

	void Engine::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowResizeEvent>([](WindowResizeEvent& resizeEvent)
		{
			CORE_TRACE("Window resized to {}x{}", resizeEvent.GetWidth(), resizeEvent.GetHeight());

			return false;
		});
	}

	bool Engine::IsRunning() const
	{
		return m_Window && m_Window->IsOpen();
	}
}