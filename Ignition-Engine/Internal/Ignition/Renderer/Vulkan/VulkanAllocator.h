#pragma once

#include <vulkan/vulkan.h>

VK_DEFINE_HANDLE(VmaAllocator)

namespace Ignition
{
	class VulkanAllocator
	{
	public:
		VulkanAllocator();
		~VulkanAllocator();

		VulkanAllocator(const VulkanAllocator&) = delete;
		VulkanAllocator& operator=(const VulkanAllocator&) = delete;

		void Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t apiVersion);
		void Shutdown();

		VmaAllocator GetAllocator() const { return m_Allocator; }

	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
	};
}