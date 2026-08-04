#pragma once

#include <memory>

namespace Ignition
{
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

		bool IsRunning() const;

	private:
		std::unique_ptr<Window> m_Window;
	};
}