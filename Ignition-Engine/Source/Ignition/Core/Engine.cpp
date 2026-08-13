#include "Ignition/Core/Engine.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Input/Input.h"
#include "Ignition/Input/InputImplementation.h"
#include "Ignition/Window/Window.h"
#include "Ignition/Window/WindowImplementation.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/RendererImplementation.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

namespace Ignition
{
	Engine::Engine() = default;
	Engine::~Engine() = default;

	void Engine::Initialize(const char* title, int width, int height)
	{
		IG_CORE_INFO("------- INITIALIZING IGNITION -------");

		m_Window = std::unique_ptr<Window>(new Window());
		m_Window->m_Implementation->Initialize(title, width, height);

		if (!m_Window->IsOpen())
		{
			IG_CORE_CRITICAL("Engine initialization failed: could not create window");
			Shutdown();

			return;
		}

		m_Input = std::unique_ptr<Input>(new Input());
		m_Input->m_Implementation->Initialize(m_Window.get());

		IG_CORE_INFO("------- INITIALIZING RENDERER -------");

		m_Backend = std::make_unique<VulkanRenderer>();
		m_Backend->Initialize(m_Window->m_Implementation->SDLWindow);

		if (!m_Backend->IsValid())
		{
			IG_CORE_CRITICAL("Engine initialization failed: could not create renderer");
			Shutdown();

			return;
		}

		IG_CORE_INFO("------- RENDERER INITIALIZED -------");

		m_Renderer = std::unique_ptr<Renderer>(new Renderer());
		m_Renderer->m_Implementation->Backend = m_Backend.get();

		m_Window->m_Implementation->RawCallback = [this](const void* sdlEvent)
		{
			if (m_Backend)
			{
				m_Backend->ProcessImGuiEvent(sdlEvent);
			}
		};

		IG_CORE_INFO("------- IGNITION INITIALIZED -------");
	}

	void Engine::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN IGNITION -------");

		// The non-owning handle must drop before the backend it points at
		m_Renderer.reset();

		if (m_Backend)
		{
			IG_CORE_INFO("------- SHUTTING DOWN RENDERER -------");

			m_Backend->Shutdown();
			m_Backend.reset();

			IG_CORE_INFO("------- RENDERER SHUTDOWN COMPLETE -------");
		}

		if (m_Input)
		{
			m_Input->m_Implementation->Shutdown();
			m_Input.reset();
		}

		if (m_Window)
		{
			m_Window->m_Implementation->Shutdown();
			m_Window.reset();
		}

		IG_CORE_INFO("------- IGNITION SHUTDOWN COMPLETE -------");
	}

	void Engine::PollEvents()
	{
		if (m_Input)
		{
			m_Input->m_Implementation->NewFrame();
		}

		if (m_Window)
		{
			m_Window->m_Implementation->PollEvents(m_EventQueue);
		}
	}

	void Engine::BeginFrame()
	{
		if (!m_Backend)
		{
			return;
		}

		m_Backend->BeginFrame();
		m_Backend->BeginImGuiFrame();
	}

	void Engine::EndFrame()
	{
		if (m_Backend)
		{
			m_Backend->EndFrame();
		}
	}

	void Engine::WaitIdle()
	{
		if (m_Backend)
		{
			m_Backend->WaitIdle();
		}
	}

	void Engine::OnEvent(Event& event)
	{
		if (m_Input)
		{
			m_Input->m_Implementation->OnEvent(event);
		}

		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& resizeEvent)
		{
			IG_CORE_TRACE("Window Resized To: {}x{} pixels", resizeEvent.GetPixelWidth(), resizeEvent.GetPixelHeight());

			if (m_Backend)
			{
				m_Backend->OnResize();
			}

			return false;
		});
	}

	bool Engine::IsRunning() const
	{
		return m_Window && m_Window->IsOpen();
	}
}