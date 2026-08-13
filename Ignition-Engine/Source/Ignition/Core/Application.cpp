#include "Ignition/Core/Application.h"

#include "Ignition/Core/ApplicationImplementation.h"
#include "Ignition/Core/Engine.h"
#include "Ignition/Core/Log.h"
#include "Ignition/UI/ImGuiLayer.h"
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

		m_Implementation = std::make_unique<ApplicationImplementation>();
		m_Implementation->Engine = std::make_unique<Engine>();
		m_Implementation->Engine->Initialize(m_Specification.Title, m_Specification.Width, m_Specification.Height);

		if (!m_Implementation->Engine->IsRunning())
		{
			IG_CORE_CRITICAL("Application initialization failed, the application will not run");
			m_Implementation.reset();

			return;
		}

		auto imguiLayer = std::make_unique<ImGuiLayer>(m_Implementation->Engine->GetRenderer(), m_Implementation->Engine->GetInput());
		m_Implementation->ImGui = imguiLayer.get();
		m_Implementation->Layers.PushOverlay(std::move(imguiLayer));

		OnInitialize();

		IG_CORE_INFO("------- APPLICATION INITIALIZED -------");
	}

	void Application::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

		if (m_Implementation && m_Implementation->Engine)
		{
			m_Implementation->Engine->WaitIdle();
		}

		OnShutdown();

		if (m_Implementation)
		{
			m_Implementation->ImGui = nullptr;
			m_Implementation->Layers.Clear();

			if (m_Implementation->Engine)
			{
				m_Implementation->Engine->Shutdown();
			}

			m_Implementation.reset();
		}

		IG_CORE_INFO("------- APPLICATION SHUTDOWN COMPLETE -------");
	}

	void Application::Run()
	{
		if (!m_Implementation || !m_Implementation->Engine)
		{
			IG_CORE_CRITICAL("Application::Run called before Initialize");

			return;
		}

		while (m_Implementation->Engine->IsRunning())
		{
			Time::Update();

			m_Implementation->Engine->PollEvents();

			const auto queuedEvents = m_Implementation->Engine->GetEventQueue().Take();

			for (const auto& queuedEvent : queuedEvents)
			{
				m_Implementation->Engine->OnEvent(*queuedEvent);

				if (!queuedEvent->IsHandled())
				{
					m_Implementation->Layers.OnEvent(*queuedEvent);
				}

				if (!queuedEvent->IsHandled())
				{
					OnEvent(*queuedEvent);
				}
			}

			while (Time::NextFixedStep())
			{
				OnFixedUpdate(Time::GetFixedTimeStep());
				m_Implementation->Layers.OnFixedUpdate(Time::GetFixedTimeStep());
			}

			OnUpdate(Time::GetDeltaTime());
			m_Implementation->Layers.OnUpdate(Time::GetDeltaTime());

			if (!m_Implementation->Engine->IsRunning())
			{
				break;
			}

			m_Implementation->Engine->BeginFrame();

			if (m_Implementation->ImGui)
			{
				m_Implementation->ImGui->BeginFrame();
			}

			OnRender();
			m_Implementation->Layers.OnRender();

			m_Implementation->Engine->EndFrame();
		}
	}

	void Application::PushLayer(std::unique_ptr<Layer> layer)
	{
		if (m_Implementation)
		{
			m_Implementation->Layers.PushLayer(std::move(layer));
		}
	}

	void Application::PushOverlay(std::unique_ptr<Layer> overlay)
	{
		if (m_Implementation)
		{
			m_Implementation->Layers.PushOverlay(std::move(overlay));
		}
	}

	Renderer* Application::GetRenderer() const
	{
		return m_Implementation && m_Implementation->Engine ? m_Implementation->Engine->GetRenderer() : nullptr;
	}

	Input* Application::GetInput() const
	{
		return m_Implementation && m_Implementation->Engine ? m_Implementation->Engine->GetInput() : nullptr;
	}

	Window* Application::GetWindow() const
	{
		return m_Implementation && m_Implementation->Engine ? m_Implementation->Engine->GetWindow() : nullptr;
	}
}