#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Scene/Entity.h"

#include <string>
#include <vector>

namespace Ignition
{
	class Renderer;
	class Scene;

	class ModelImporter
	{
	public:
		IGNITION_API static std::vector<Entity> Import(Scene& scene, Renderer& renderer, const std::string& filepath);
	};
}