#pragma once

#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

namespace Ignition
{
	struct RendererImplementation
	{
		VulkanRenderer* Backend = nullptr;
	};
}