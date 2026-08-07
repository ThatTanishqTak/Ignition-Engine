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
			static bool VKCheck(VkResult result, const char* message);
		};
	}
}