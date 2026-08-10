#include "Ignition/Core/Engine.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Input/Input.h"
#include "Ignition/Window/Window.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Core/Time.h"

#include <imgui.h>

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

		m_Input = std::make_unique<Input>();
		m_Input->Initialize(m_Window.get());

		m_Renderer = std::make_unique<Renderer>();
		m_Renderer->Initialize(m_Window->GetNativeWindow());

		if (!m_Renderer->IsValid())
		{
			IG_CORE_CRITICAL("Engine initialization failed: could not create renderer");
			Shutdown();

			return;
		}

		m_Window->SetRawEventCallback([this](const void* sdlEvent)
		{
			if (m_Renderer)
			{
				m_Renderer->ProcessImGuiEvent(sdlEvent);
			}
		});

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

		if (m_Input)
		{
			m_Input->Shutdown();
			m_Input.reset();
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
		if (m_Input)
		{
			m_Input->NewFrame();
		}

		if (m_Window)
		{
			m_Window->PollEvents(m_EventQueue);
		}
	}

	void Engine::BeginFrame()
	{
		if (!m_Renderer)
		{
			return;
		}

		m_Renderer->BeginFrame();
		m_Renderer->BeginImGuiFrame();

		if (m_Renderer->IsImGuiFrameActive())
		{
			ImGui::DockSpaceOverViewport(0, nullptr, ImGuiDockNodeFlags_PassthruCentralNode);

			DrawDebugWindows();
		}
	}

	void Engine::EndFrame()
	{
		if (m_Renderer)
		{
			m_Renderer->EndFrame();
		}
	}

	void Engine::DrawDebugWindows()
	{
		ImGui::Begin("Stats");
		ImGui::Text("FPS: %.1f", Time::GetFramesPerSecond());
		ImGui::Text("Frame Time: %.2f ms", Time::GetAverageFrameTimeMilliseconds());
		ImGui::End();

		ImGui::Begin("Gamepads");

		const auto gamepads = m_Input->GetConnectedGamepads();

		if (gamepads.empty())
		{
			ImGui::TextUnformatted("No gamepads connected");
		}

		for (const GamepadID gamepadID : gamepads)
		{
			ImGui::Text("[%d] %s", static_cast<int>(gamepadID), m_Input->GetGamepadName(gamepadID).c_str());
		}

		ImGui::End();
	}

	void Engine::OnEvent(Event& event)
	{
		if (m_Input)
		{
			m_Input->OnEvent(event);
		}

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