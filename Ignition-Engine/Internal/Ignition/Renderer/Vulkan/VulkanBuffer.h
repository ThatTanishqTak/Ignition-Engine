#pragma once

#include <vulkan/vulkan.h>

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

namespace Ignition
{
	enum class VulkanBufferAccess
	{
		DeviceLocal = 0, // GPU only, filled through a staging copy
		HostWrite,       // mapped for sequential CPU writes (staging, per-frame vertex data)
		HostRead         // mapped for CPU reads of GPU results (compute readback)
	};

	class VulkanBuffer
	{
	public:
		VulkanBuffer();
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		void Initialize(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VulkanBufferAccess access = VulkanBufferAccess::DeviceLocal);
		void Shutdown();

		bool IsValid() const { return m_Buffer != VK_NULL_HANDLE; }

		VkBuffer GetBuffer() const { return m_Buffer; }
		VkDeviceSize GetSize() const { return m_Size; }

		void* Map();
		void Unmap();

		// Makes device writes visible to the host before a mapped read
		void Invalidate();

	private:
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceSize m_Size = 0;
		VulkanBufferAccess m_Access = VulkanBufferAccess::DeviceLocal;
	};
}