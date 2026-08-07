#include "Ignition/Core/Application.h"

#include "Ignition/Core/Engine.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Core/Time.h"
#include "Ignition/Events/EventQueue.h"

namespace Ignition
{
	Application::Application() = default;

	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{

	}

	Application::~Application() = default;

	void Application::Initialize()
	{
		IG_CORE_INFO("------- INITIALIZING APPLICATION -------");

		m_Engine = std::make_unique<Engine>();
		m_Engine->Initialize(m_Specification.Title, m_Specification.Width, m_Specification.Height);

		if (!m_Engine->IsRunning())
		{
			IG_CORE_CRITICAL("Application initialization failed, the application will not run");
			m_Engine.reset();

			return;
		}

		OnInitialize();

		IG_CORE_INFO("------- APPLICATION INITIALIZED -------");
	}

	void Application::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

		OnShutdown();

		if (m_Engine)
		{
			m_Engine->Shutdown();
			m_Engine.reset();
		}

		IG_CORE_INFO("------- APPLICATION SHUTDOWN COMPLETE -------");
	}

	void Application::Run()
	{
		if (!m_Engine)
		{
			IG_CORE_CRITICAL("Application::Run called before Initialize");

			return;
		}

		while (m_Engine->IsRunning())
		{
			Time::Update();

			m_Engine->PollEvents();

			const auto queuedEvents = m_Engine->GetEventQueue().Take();

			for (const auto& queuedEvent : queuedEvents)
			{
				m_Engine->OnEvent(*queuedEvent);

				if (!queuedEvent->m_Handled)
				{
					OnEvent(*queuedEvent);
				}
			}

			while (Time::NextFixedStep())
			{
				OnFixedUpdate(Time::GetFixedTimeStep());
			}

			OnUpdate(Time::GetDeltaTime());

			if (!m_Engine->IsRunning())
			{
				break;
			}

			m_Engine->Render();
		}
	}
}