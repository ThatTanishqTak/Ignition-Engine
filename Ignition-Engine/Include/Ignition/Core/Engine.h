#pragma once

#include "Ignition/Events/EventQueue.h"

#include <memory>

namespace Ignition
{
	class Event;
	class Window;
	class Renderer;
	class Input;

	class Engine
	{
	public:
		Engine();
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		void Initialize(const char* title, int width, int height);
		void Shutdown();

		void PollEvents();

		void BeginFrame();
		void EndFrame();

		void OnEvent(Event& event);

		EventQueue& GetEventQueue() { return m_EventQueue; }
		Renderer* GetRenderer() const { return m_Renderer.get(); }
		Input* GetInput() const { return m_Input.get(); }

		bool IsRunning() const;

	private:
		void DrawDebugWindows();

	private:
		std::unique_ptr<Window> m_Window;
		std::unique_ptr<Input> m_Input;
		std::unique_ptr<Renderer> m_Renderer;
		EventQueue m_EventQueue;
	};
}