#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include "Ignition/Core/Log.h"

#include <cstdlib>

namespace Ignition
{
	namespace Utilities
	{
		void VulkanUtilities::VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				IG_CORE_CRITICAL("[VULKAN]: {}", message);
				std::abort();
			}
		}
	}
}