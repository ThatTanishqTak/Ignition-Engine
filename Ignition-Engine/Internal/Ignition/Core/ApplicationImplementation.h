#pragma once

#include "Ignition/Core/Engine.h"
#include "Ignition/Core/LayerStack.h"

#include <memory>

namespace Ignition
{
	class UILayer;

	struct ApplicationImplementation
	{
		std::unique_ptr<Engine> Engine;
		LayerStack Layers;

		// Non-owning: the stack owns the layer, this is the handle the host uses to reach its context
		UILayer* UI = nullptr;
	};
}