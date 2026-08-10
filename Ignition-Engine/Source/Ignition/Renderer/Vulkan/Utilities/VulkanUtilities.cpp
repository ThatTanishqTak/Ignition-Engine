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

		bool VulkanUtilities::SubmitOneShotCommands(VkDevice device, VkQueue queue, uint32_t queueFamily, const std::function<void(VkCommandBuffer)>& record)
		{
			VkCommandPoolCreateInfo poolCreateInfo{};
			poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolCreateInfo.queueFamilyIndex = queueFamily;

			VkCommandPool commandPool = VK_NULL_HANDLE;

			if (VulkanUtilities::VKCheck(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool), "Failed vkCreateCommandPool"))
			{
				return false;
			}

			VkCommandBufferAllocateInfo allocateInfo{};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.commandPool = commandPool;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

			if (VulkanUtilities::VKCheck(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer), "Failed vkAllocateCommandBuffers"))
			{
				vkDestroyCommandPool(device, commandPool, nullptr);

				return false;
			}

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			bool succeeded = false;

			if (!VulkanUtilities::VKCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed vkBeginCommandBuffer"))
			{
				record(commandBuffer);

				if (!VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "Failed vkEndCommandBuffer"))
				{
					VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
					commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
					commandBufferSubmitInfo.commandBuffer = commandBuffer;

					VkSubmitInfo2 submitInfo{};
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
					submitInfo.commandBufferInfoCount = 1;
					submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

					if (!VulkanUtilities::VKCheck(vkQueueSubmit2(queue, 1, &submitInfo, VK_NULL_HANDLE), "Failed vkQueueSubmit2"))
					{
						succeeded = !VulkanUtilities::VKCheck(vkQueueWaitIdle(queue), "Failed vkQueueWaitIdle");
					}
				}
			}

			vkDestroyCommandPool(device, commandPool, nullptr);

			return succeeded;
		}
	}
}