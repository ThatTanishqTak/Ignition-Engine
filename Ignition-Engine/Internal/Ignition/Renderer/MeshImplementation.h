#pragma once

#include "Ignition/Renderer/Vulkan/VulkanMesh.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

#include <memory>

namespace Ignition
{
	struct MeshImplementation
	{
		std::unique_ptr<VulkanMesh> Handle;
		std::weak_ptr<VulkanRenderer*> Backend;
	};
}