#pragma once

#include "Ignition/Core/Export.h"

#include <vulkan/vulkan.h>

namespace Ignition
{
	namespace Utilities
	{
		class IGNITION_API VulkanUtilities
		{
		public:
			static void VKCheck(VkResult result, const char* message);
		};
	}
}