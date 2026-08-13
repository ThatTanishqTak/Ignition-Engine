#pragma once

#include <vulkan/vulkan.h>

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

namespace Ignition
{
	class VulkanBuffer
	{
	public:
		VulkanBuffer();
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		void Initialize(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, bool hostVisible);
		void Shutdown();

		bool IsValid() const { return m_Buffer != VK_NULL_HANDLE; }

		VkBuffer GetBuffer() const { return m_Buffer; }
		VkDeviceSize GetSize() const { return m_Size; }

		void* Map();
		void Unmap();

	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceSize m_Size = 0;
	};
}