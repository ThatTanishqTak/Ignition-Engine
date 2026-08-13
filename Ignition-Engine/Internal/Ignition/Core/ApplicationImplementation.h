#pragma once

#include "Ignition/Core/Engine.h"
#include "Ignition/Core/LayerStack.h"

#include <memory>

namespace Ignition
{
	class ImGuiLayer;

	struct ApplicationImplementation
	{
		std::unique_ptr<Engine> Engine;
		LayerStack Layers;
		ImGuiLayer* ImGui = nullptr;
	};
}