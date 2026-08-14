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
			static bool CheckVkResult(VkResult result, const char* expression, const char* file, int line);
			static bool SubmitOneShotCommands(VkDevice device, VkQueue queue, uint32_t queueFamily, const std::function<void(VkCommandBuffer)>& record);
			static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);
		};
	}
}

#define VK_CHECK(expression) ::Ignition::Utilities::VulkanUtilities::CheckVkResult((expression), #expression, __FILE__, __LINE__)