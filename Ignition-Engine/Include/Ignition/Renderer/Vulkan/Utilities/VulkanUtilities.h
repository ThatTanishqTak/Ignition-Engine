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
			static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);
		};
	}
}