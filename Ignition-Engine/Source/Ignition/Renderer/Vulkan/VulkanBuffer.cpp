#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <vk_mem_alloc.h>

namespace Ignition
{
	VulkanBuffer::VulkanBuffer() = default;
	VulkanBuffer::~VulkanBuffer() = default;

	void VulkanBuffer::Initialize(VmaAllocator allocator, VkDeviceSize size, VkBufferUsageFlags usage, VulkanBufferAccess access)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN BUFFER -------");

		m_Allocator = allocator;
		m_Size = size;
		m_Access = access;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = size;
		bufferCreateInfo.usage = usage;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (access == VulkanBufferAccess::HostWrite)
		{
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
		}
		else if (access == VulkanBufferAccess::HostRead)
		{
			// Readback wants cached memory, not the write-combined kind sequential writes get
			allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
		}

		if (!VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferCreateInfo, &allocationCreateInfo, &m_Buffer, &m_Allocation, nullptr)))
		{
			m_Buffer = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
			m_Size = 0;
		}

		IG_CORE_INFO("------- VULKAN BUFFER INIITIALIZED -------");
	}

	void VulkanBuffer::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN BUFFER -------");

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vmaDestroyBuffer(m_Allocator, m_Buffer, m_Allocation);
		}

		m_Buffer = VK_NULL_HANDLE;
		m_Allocation = VK_NULL_HANDLE;
		m_Allocator = VK_NULL_HANDLE;
		m_Size = 0;
		m_Access = VulkanBufferAccess::DeviceLocal;

		IG_CORE_INFO("------- VULKAN BUFFER SHUTDOWN COMPLETE -------");
	}

	void* VulkanBuffer::Map()
	{
		void* data = nullptr;

		if (!VK_CHECK(vmaMapMemory(m_Allocator, m_Allocation, &data)))
		{
			return nullptr;
		}

		return data;
	}

	void VulkanBuffer::Unmap()
	{
		if (m_Access == VulkanBufferAccess::HostWrite)
		{
			vmaFlushAllocation(m_Allocator, m_Allocation, 0, VK_WHOLE_SIZE);
		}

		vmaUnmapMemory(m_Allocator, m_Allocation);
	}

	void VulkanBuffer::Invalidate()
	{
		if (m_Access == VulkanBufferAccess::HostRead && m_Allocation != VK_NULL_HANDLE)
		{
			vmaInvalidateAllocation(m_Allocator, m_Allocation, 0, VK_WHOLE_SIZE);
		}
	}
}