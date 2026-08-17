#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <vector>

namespace Ignition
{
	namespace Utilities
	{
		class IGNITION_API VulkanUtilities
		{
		public:
			static bool CheckVkResult(VkResult result, const char* expression, const char* file, int line);
			static bool SubmitOneShotCommands(VkDevice device, VkQueue queue, uint32_t queueFamily, const std::function<void(VkCommandBuffer)>& record);
			static bool UploadViaStaging(VkDevice device, VkQueue queue, uint32_t queueFamily, VmaAllocator allocator, const void* data, VkDeviceSize size, const std::function<void(VkCommandBuffer, const VulkanBuffer& staging)>& recordCopy);
			static void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);

			static VkShaderModule CreateShaderModule(VkDevice device, const std::string& spirvPath);

			static void MemoryBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);
			static void BufferBarrier(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, VkDeviceSize size, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);

			// The two patterns compute work repeats constantly
			static void ComputeToComputeBarrier(VkCommandBuffer commandBuffer);   // ping-pong: this dispatch writes, the next reads
			static void ComputeToGraphicsBarrier(VkCommandBuffer commandBuffer);  // the sim writes a field, a draw samples it
		};
	}
}

#define VK_CHECK(expression) ::Ignition::Utilities::VulkanUtilities::CheckVkResult((expression), #expression, __FILE__, __LINE__)

namespace Ignition
{
	namespace Utilities
	{
		template <typename T, typename F, typename... Args>
		std::vector<T> Enumerate(F&& fn, Args&&... args)
		{
			const auto invoke = [&](uint32_t* count, T* items)
			{
				if constexpr (std::is_void_v<std::invoke_result_t<F&, Args&..., uint32_t*, T*>>)
				{
					fn(args..., count, items);
				}
				else
				{
					VK_CHECK(fn(args..., count, items));
				}
			};

			uint32_t count = 0;
			invoke(&count, nullptr);

			std::vector<T> items(count);
			invoke(&count, items.data());

			return items;
		}
	}
}