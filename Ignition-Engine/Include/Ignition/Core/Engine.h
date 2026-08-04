#pragma once

#include "Ignition/Events/EventQueue.h"

#include <memory>

namespace Ignition
{
	class Event;
	class Window;

	class Engine
	{
	public:
		Engine();
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		void Initialize(const char* title, int width, int height);
		void Shutdown();

		void Update();

		void OnEvent(Event& event);

		EventQueue& GetEventQueue() { return m_EventQueue; }

		bool IsRunning() const;

	private:
		std::unique_ptr<Window> m_Window;
		EventQueue m_EventQueue;
	};
}