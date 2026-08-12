#include "Ignition/Core/Application.h"

#include "Ignition/Core/Engine.h"
#include "Ignition/Core/Log.h"
#include "Ignition/UI/ImGuiLayer.h"
#include "Ignition/Core/Time.h"
#include "Ignition/Events/EventQueue.h"
#include "Ignition/Renderer/Renderer.h"

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

		auto imguiLayer = std::make_unique<ImGuiLayer>(m_Engine->GetRenderer(), m_Engine->GetInput());
		m_ImGuiLayer = imguiLayer.get();
		m_LayerStack.PushOverlay(std::move(imguiLayer));

		OnInitialize();

		IG_CORE_INFO("------- APPLICATION INITIALIZED -------");
	}

	void Application::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN APPLICATION -------");

		if (GetRenderer())
		{
			GetRenderer()->WaitIdle();
		}

		OnShutdown();

		m_ImGuiLayer = nullptr;
		m_LayerStack.Clear();

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

				if (!queuedEvent->IsHandled())
				{
					m_LayerStack.OnEvent(*queuedEvent);
				}

				if (!queuedEvent->IsHandled())
				{
					OnEvent(*queuedEvent);
				}
			}

			while (Time::NextFixedStep())
			{
				OnFixedUpdate(Time::GetFixedTimeStep());
				m_LayerStack.OnFixedUpdate(Time::GetFixedTimeStep());
			}

			OnUpdate(Time::GetDeltaTime());
			m_LayerStack.OnUpdate(Time::GetDeltaTime());

			if (!m_Engine->IsRunning())
			{
				break;
			}

			m_Engine->BeginFrame();

			if (m_ImGuiLayer)
			{
				m_ImGuiLayer->BeginFrame();
			}

			OnRender();
			m_LayerStack.OnRender();

			m_Engine->EndFrame();
		}
	}

	void Application::PushLayer(std::unique_ptr<Layer> layer)
	{
		m_LayerStack.PushLayer(std::move(layer));
	}

	void Application::PushOverlay(std::unique_ptr<Layer> overlay)
	{
		m_LayerStack.PushOverlay(std::move(overlay));
	}

	Renderer* Application::GetRenderer() const
	{
		return m_Engine ? m_Engine->GetRenderer() : nullptr;
	}

	Input* Application::GetInput() const
	{
		return m_Engine ? m_Engine->GetInput() : nullptr;
	}

	Window* Application::GetWindow() const
	{
		return m_Engine ? m_Engine->GetWindow() : nullptr;
	}
}