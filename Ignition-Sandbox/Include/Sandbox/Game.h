#pragma once

#include "Ignition/Core/Engine.h"

#include <memory>

namespace Game
{
	class GameApplication
	{
	public:
		void Initialize();
		void Shutdown();

		void Run();

	private:
		std::unique_ptr<Ignition::Engine> m_Engine;
	};
}