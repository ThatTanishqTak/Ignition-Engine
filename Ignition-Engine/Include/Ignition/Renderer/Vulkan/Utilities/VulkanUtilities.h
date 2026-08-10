#pragma once

#include "Ignition/Core/Export.h"

#include <vulkan/vulkan.h>

#include <functional>

namespace Ignition
{
	namespace Utilities
	{
		class IGNITION_API VulkanUtilities
		{
		public:
			static bool VKCheck(VkResult result, const char* message);
			static bool SubmitOneShotCommands(VkDevice device, VkQueue queue, uint32_t queueFamily, const std::function<void(VkCommandBuffer)>& record);
		};
	}
}