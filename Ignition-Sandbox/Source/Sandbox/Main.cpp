#include "Sandbox/SandboxApplication.h"

#include "Ignition/Ignition.h"
#include "Ignition/Core/EntryPoint.h"

#include <memory>

namespace Ignition
{
	std::unique_ptr<Application> CreateApplication(int argc, char** argv)
	{
		(void)argc;
		(void)argv;

		return std::make_unique<Sandbox::SandboxApplication>();
	}
}