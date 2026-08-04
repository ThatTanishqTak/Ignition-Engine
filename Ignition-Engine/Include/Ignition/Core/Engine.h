#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	class Window;

	class IGNITION_API Engine
	{
	public:
		Engine();
		~Engine();

		Engine(const Engine&) = delete;
		Engine& operator=(const Engine&) = delete;

		void Initialize();
		void Shutdown();

		void Run();

	private:
		std::unique_ptr<Window> m_Window;
	};
}