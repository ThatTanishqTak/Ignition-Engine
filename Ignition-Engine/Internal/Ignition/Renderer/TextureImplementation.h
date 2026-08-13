#pragma once

#include "Ignition/Renderer/Vulkan/VulkanTexture.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

#include <memory>

namespace Ignition
{
	struct TextureImplementation
	{
		std::unique_ptr<VulkanTexture> Handle;
		std::weak_ptr<VulkanRenderer*> Backend;
	};
}