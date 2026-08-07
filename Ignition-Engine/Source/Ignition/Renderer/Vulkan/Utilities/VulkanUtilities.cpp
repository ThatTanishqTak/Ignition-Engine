#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include "Ignition/Core/Log.h"

namespace Ignition
{
	namespace Utilities
	{
		bool VulkanUtilities::VKCheck(VkResult result, const char* message)
		{
			if (result < 0)
			{
				IG_CORE_ERROR("[VULKAN]: {} (VkResult: {})", message, static_cast<int>(result));

				return true;
			}

			if (result != VK_SUCCESS)
			{
				IG_CORE_TRACE("[VULKAN]: {} (VkResult: {})", message, static_cast<int>(result));
			}

			return false;
		}
	}
}