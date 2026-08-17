#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Core/Utilities.h"

#include <cstring>

namespace Ignition
{
	namespace Utilities
	{
		bool VulkanUtilities::CheckVkResult(VkResult result, const char* expression, const char* file, int line)
		{
			if (result < 0)
			{
				IG_CORE_ERROR("[VULKAN]: {} failed (VkResult: {}) at {}:{}", expression, static_cast<int>(result), file, line);

				return false;
			}

			if (result != VK_SUCCESS)
			{
				IG_CORE_TRACE("[VULKAN]: {} returned VkResult {} at {}:{}", expression, static_cast<int>(result), file, line);
			}

			return true;
		}

		bool VulkanUtilities::SubmitOneShotCommands(VkDevice device, VkQueue queue, uint32_t queueFamily, const std::function<void(VkCommandBuffer)>& record)
		{
			VkCommandPoolCreateInfo poolCreateInfo{};
			poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolCreateInfo.queueFamilyIndex = queueFamily;

			VkCommandPool commandPool = VK_NULL_HANDLE;

			if (!VK_CHECK(vkCreateCommandPool(device, &poolCreateInfo, nullptr, &commandPool)))
			{
				return false;
			}

			VkCommandBufferAllocateInfo allocateInfo{};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.commandPool = commandPool;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocateInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

			if (!VK_CHECK(vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer)))
			{
				vkDestroyCommandPool(device, commandPool, nullptr);

				return false;
			}

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			bool succeeded = false;

			if (VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo)))
			{
				record(commandBuffer);

				if (VK_CHECK(vkEndCommandBuffer(commandBuffer)))
				{
					VkCommandBufferSubmitInfo commandBufferSubmitInfo{};
					commandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
					commandBufferSubmitInfo.commandBuffer = commandBuffer;

					VkSubmitInfo2 submitInfo{};
					submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
					submitInfo.commandBufferInfoCount = 1;
					submitInfo.pCommandBufferInfos = &commandBufferSubmitInfo;

					if (VK_CHECK(vkQueueSubmit2(queue, 1, &submitInfo, VK_NULL_HANDLE)))
					{
						succeeded = VK_CHECK(vkQueueWaitIdle(queue));
					}
				}
			}

			vkDestroyCommandPool(device, commandPool, nullptr);

			return succeeded;
		}

		bool VulkanUtilities::UploadViaStaging(VkDevice device, VkQueue queue, uint32_t queueFamily, VmaAllocator allocator, const void* data, VkDeviceSize size, const std::function<void(VkCommandBuffer, const VulkanBuffer& staging)>& recordCopy)
		{
			VulkanBuffer stagingBuffer;
			stagingBuffer.Initialize(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VulkanBufferAccess::HostWrite);

			if (!stagingBuffer.IsValid())
			{
				return false;
			}

			void* mapped = stagingBuffer.Map();

			if (!mapped)
			{
				stagingBuffer.Shutdown();

				return false;
			}

			std::memcpy(mapped, data, static_cast<size_t>(size));
			stagingBuffer.Unmap();

			const bool submitted = SubmitOneShotCommands(device, queue, queueFamily, [&](VkCommandBuffer commandBuffer)
			{
				recordCopy(commandBuffer, stagingBuffer);
			});

			stagingBuffer.Shutdown();

			return submitted;
		}

		void VulkanUtilities::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess)
		{
			VkImageMemoryBarrier2 imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
			imageMemoryBarrier.srcStageMask = sourceStage;
			imageMemoryBarrier.srcAccessMask = sourceAccess;
			imageMemoryBarrier.dstStageMask = destinationStage;
			imageMemoryBarrier.dstAccessMask = destinationAccess;
			imageMemoryBarrier.oldLayout = oldLayout;
			imageMemoryBarrier.newLayout = newLayout;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange.aspectMask = aspectMask;
			imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
			imageMemoryBarrier.subresourceRange.levelCount = 1;
			imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
			imageMemoryBarrier.subresourceRange.layerCount = 1;

			VkDependencyInfo dependencyInfo{};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = 1;
			dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrier;

			vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
		}

		VkShaderModule VulkanUtilities::CreateShaderModule(VkDevice device, const std::string& spirvPath)
		{
			const std::vector<char> code = CoreUtilities::ReadBinaryFile(spirvPath);

			if (code.empty())
			{
				return VK_NULL_HANDLE;
			}

			VkShaderModuleCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = code.size();
			createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

			VkShaderModule shaderModule = VK_NULL_HANDLE;

			if (!VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule)))
			{
				return VK_NULL_HANDLE;
			}

			return shaderModule;
		}

		void VulkanUtilities::MemoryBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess)
		{
			VkMemoryBarrier2 memoryBarrier{};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
			memoryBarrier.srcStageMask = sourceStage;
			memoryBarrier.srcAccessMask = sourceAccess;
			memoryBarrier.dstStageMask = destinationStage;
			memoryBarrier.dstAccessMask = destinationAccess;

			VkDependencyInfo dependencyInfo{};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.memoryBarrierCount = 1;
			dependencyInfo.pMemoryBarriers = &memoryBarrier;

			vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
		}

		void VulkanUtilities::BufferBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess)
		{
			VkBufferMemoryBarrier2 bufferMemoryBarrier{};
			bufferMemoryBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			bufferMemoryBarrier.srcStageMask = sourceStage;
			bufferMemoryBarrier.srcAccessMask = sourceAccess;
			bufferMemoryBarrier.dstStageMask = destinationStage;
			bufferMemoryBarrier.dstAccessMask = destinationAccess;
			bufferMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufferMemoryBarrier.buffer = buffer;
			bufferMemoryBarrier.offset = offset;
			bufferMemoryBarrier.size = size;

			VkDependencyInfo dependencyInfo{};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.bufferMemoryBarrierCount = 1;
			dependencyInfo.pBufferMemoryBarriers = &bufferMemoryBarrier;

			vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
		}

		void VulkanUtilities::ComputeToComputeBarrier(VkCommandBuffer commandBuffer)
		{
			MemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT);
		}

		void VulkanUtilities::ComputeToGraphicsBarrier(VkCommandBuffer commandBuffer)
		{
			MemoryBarrier(commandBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT, VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);
		}
	}
}