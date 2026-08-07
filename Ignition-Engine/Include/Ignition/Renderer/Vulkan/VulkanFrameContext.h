#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Ignition
{
	class VulkanFrameContext
	{
	public:
		static constexpr uint32_t MaximumFramesInFlight = 2;

		VulkanFrameContext();
		~VulkanFrameContext();

		VulkanFrameContext(const VulkanFrameContext&) = delete;
		VulkanFrameContext& operator=(const VulkanFrameContext&) = delete;

		void Initialize(VkDevice device, uint32_t graphicsQueueFamily);
		void Shutdown();

		bool IsValid() const;

		VkCommandBuffer GetCommandBuffer(uint32_t frameIndex) const { return m_CommandBuffers[frameIndex]; }
		VkSemaphore GetImageAvailableSemaphore(uint32_t frameIndex) const { return m_ImageAvailableSemaphores[frameIndex]; }
		VkFence GetInFlightFence(uint32_t frameIndex) const { return m_InFlightFences[frameIndex]; }

	private:
		void CreateCommandPool(uint32_t graphicsQueueFamily);
		void AllocateCommandBuffers();
		void CreateSyncObjects();

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;

		std::vector<VkCommandBuffer> m_CommandBuffers;
		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkFence> m_InFlightFences;
	};
}