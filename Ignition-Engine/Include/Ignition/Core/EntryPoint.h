#pragma once

#include "Ignition/Core/Application.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Core/Profiler.h"
#include "Ignition/Core/Time.h"

int main(int argc, char** argv)
{
	Ignition::Log::Initialize();
	Ignition::Profiler::Initialize();
	Ignition::Time::Initialize();

	auto application = Ignition::CreateApplication(argc, argv);

	if (!application)
	{
		IG_APP_CRITICAL("CreateApplication returned null");
		Ignition::Profiler::Shutdown();
		Ignition::Log::Shutdown();

		return 1;
	}

	application->Initialize();
	application->Run();
	application->Shutdown();

	application.reset();

	Ignition::Profiler::Shutdown();
	Ignition::Log::Shutdown();

	return 0;
}