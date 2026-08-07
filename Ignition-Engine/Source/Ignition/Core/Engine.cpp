#include "Ignition/Core/Engine.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Window/Window.h"
#include "Ignition/Renderer/Renderer.h"

namespace Ignition
{
	Engine::Engine() = default;
	Engine::~Engine() = default;

	void Engine::Initialize(const char* title, int width, int height)
	{
		IG_CORE_INFO("------- INITIALIZING IGNITION -------");

		m_Window = std::make_unique<Window>();
		m_Window->Initialize(title, width, height);

		if (!m_Window->IsOpen())
		{
			IG_CORE_CRITICAL("Engine initialization failed: could not create window");
			Shutdown();

			return;
		}

		m_Renderer = std::make_unique<Renderer>();
		m_Renderer->Initialize(m_Window->GetNativeWindow());

		if (!m_Renderer->IsValid())
		{
			IG_CORE_CRITICAL("Engine initialization failed: could not create renderer");
			Shutdown();

			return;
		}

		IG_CORE_INFO("------- IGNITION INITIALIZED -------");
	}

	void Engine::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN IGNITION -------");

		if (m_Renderer)
		{
			m_Renderer->Shutdown();
			m_Renderer.reset();
		}

		if (m_Window)
		{
			m_Window->Shutdown();
			m_Window.reset();
		}

		IG_CORE_INFO("------- IGNITION SHUTDOWN COMPLETE -------");
	}

	void Engine::PollEvents()
	{
		if (m_Window)
		{
			m_Window->PollEvents(m_EventQueue);
		}
	}

	void Engine::Render()
	{
		if (m_Renderer)
		{
			m_Renderer->DrawFrame(0.01f, 0.01f, 0.01f);
		}
	}

	void Engine::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& resizeEvent)
		{
			IG_CORE_TRACE("Window Resized To: {}x{} pixels", resizeEvent.GetPixelWidth(), resizeEvent.GetPixelHeight());

			if (m_Renderer)
			{
				m_Renderer->OnResize();
			}

			return false;
		});
	}

	bool Engine::IsRunning() const
	{
		return m_Window && m_Window->IsOpen();
	}
}