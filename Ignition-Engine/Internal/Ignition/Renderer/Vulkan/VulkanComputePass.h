#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Ignition
{
	class VulkanGPUTimer;

	class VulkanComputePass
	{
	public:
		virtual ~VulkanComputePass() = default;

		virtual void RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer) = 0;
	};
}