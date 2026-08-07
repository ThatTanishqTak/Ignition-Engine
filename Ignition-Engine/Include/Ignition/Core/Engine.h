#pragma once

#include "Ignition/Events/EventQueue.h"

#include <memory>

namespace Ignition
{
	class Event;
	class Window;
	class Renderer;

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
		void Render();

		void OnEvent(Event& event);

		EventQueue& GetEventQueue() { return m_EventQueue; }

		bool IsRunning() const;

	private:
		std::unique_ptr<Window> m_Window;
		std::unique_ptr<Renderer> m_Renderer;
		EventQueue m_EventQueue;
	};
}