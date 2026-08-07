#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include "Ignition/Core/Log.h"

namespace Ignition
{
	namespace Utilities
	{
		VkResult VulkanUtilities::VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				IG_CORE_ERROR("[VULKAN]: {} (VkResult: {})", message, static_cast<int>(result));
			}

			return result;
		}
	}
}