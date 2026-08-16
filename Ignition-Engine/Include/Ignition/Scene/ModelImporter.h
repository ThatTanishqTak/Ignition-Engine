#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Scene/Entity.h"

#include <string>
#include <vector>

namespace Ignition
{
	class AssetRegistry;
	class Scene;

	class ModelImporter
	{
	public:
		IGNITION_API static std::vector<Entity> Import(Scene& scene, AssetRegistry& assets, const std::string& filepath);
	};
}