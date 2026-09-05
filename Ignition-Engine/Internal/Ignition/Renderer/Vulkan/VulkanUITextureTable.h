#pragma once

#include "Ignition/Renderer/Vulkan/VulkanImage.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <deque>
#include <vector>

VK_DEFINE_HANDLE(VmaAllocator)

namespace Ignition
{
	class VulkanUITextureTable
	{
	public:
		static constexpr uint32_t RequestedCapacity = 1024;

		VulkanUITextureTable();
		~VulkanUITextureTable();

		VulkanUITextureTable(const VulkanUITextureTable&) = delete;
		VulkanUITextureTable& operator=(const VulkanUITextureTable&) = delete;

		void Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VkSampler sampler);
		void Shutdown();

		bool IsValid() const { return m_DescriptorSet != VK_NULL_HANDLE; }

		VkDescriptorSetLayout GetSetLayout() const { return m_SetLayout; }
		VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

		// Slot 0 is the permanent 1x1 white and is never handed out, so 0 doubles as the failure value
		uint32_t Acquire(VkImageView imageView);
		void Release(uint32_t slot, uint64_t frameNumber);

		void ProcessRetirement(uint64_t frameNumber);

	private:
		void Write(uint32_t slot, VkImageView imageView);
		bool CreateWhiteSlot(VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator);

	private:
		struct PendingRelease
		{
			uint32_t Slot = 0;
			uint64_t FrameNumber = 0;
		};

		VkDevice m_Device = VK_NULL_HANDLE;
		VkSampler m_Sampler = VK_NULL_HANDLE;

		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_SetLayout = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;

		VulkanImage m_WhiteImage;

		uint32_t m_Capacity = 0;
		uint32_t m_NextSlot = 1;

		std::vector<uint32_t> m_FreeSlots;
		std::deque<PendingRelease> m_PendingReleases;
	};
}