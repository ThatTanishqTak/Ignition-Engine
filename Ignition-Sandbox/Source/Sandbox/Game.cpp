#include "Sandbox/Game.h"

#include "Ignition/Core/Log.h"

namespace Game
{
	void GameApplication::Initialize()
	{
		APP_INFO("------- INITIALIZING GAME -------");

		m_Engine = std::make_unique<Ignition::Engine>();
		m_Engine->Initialize();

		APP_INFO("------- GAME INITIALIZED -------");
	}

	void GameApplication::Shutdown()
	{
		APP_INFO("------- SHUTTING DOWN GAME -------");

		m_Engine->Shutdown();
		m_Engine.reset();

		APP_INFO("------- GAME SHUTDOWN COMPLETE -------");
	}

	void GameApplication::Run()
	{
		m_Engine->Run();
	}
}

int main(int argc, char** argv)
{
	(void)argc;
	(void)argv;

	Ignition::Log::Initialize();
	Game::GameApplication application{};

	application.Initialize();
	application.Run();
	application.Shutdown();

	Ignition::Log::Shutdown();

	return 0;
}