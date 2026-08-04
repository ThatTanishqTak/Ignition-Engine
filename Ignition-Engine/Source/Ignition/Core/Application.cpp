#include "Ignition/Core/Application.h"

#include "Ignition/Core/Engine.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Core/Time.h"

namespace Ignition
{
	Application::Application() = default;

	Application::Application(const ApplicationSpecification& specification) : m_Specification(specification)
	{

	}

	Application::~Application() = default;

	void Application::Initialize()
	{
		CORE_INFO("------- INITIALIZING APPLICATION -------");

		m_Engine = std::make_unique<Engine>();
		m_Engine->Initialize(m_Specification.Title, m_Specification.Width, m_Specification.Height);

		OnInitialize();

		CORE_INFO("------- APPLICATION INITIALIZED -------");
	}

	void Application::Shutdown()
	{
		CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

		OnShutdown();

		if (m_Engine)
		{
			m_Engine->Shutdown();
			m_Engine.reset();
		}

		CORE_INFO("------- APPLICATION SHUTDOWN COMPLETE -------");
	}

	void Application::Run()
	{
		if (!m_Engine)
		{
			CORE_CRITICAL("Application::Run called before Initialize");

			return;
		}

		while (m_Engine->IsRunning())
		{
			Time::Update();

			m_Engine->Update();

			while (Time::NextFixedStep())
			{
				OnFixedUpdate(Time::GetFixedTimeStep());
			}

			OnUpdate(Time::GetDeltaTime());
		}
	}
}