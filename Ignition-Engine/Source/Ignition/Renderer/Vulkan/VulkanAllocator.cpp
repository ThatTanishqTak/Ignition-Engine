#include "Ignition/Renderer/Vulkan/VulkanAllocator.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace Ignition
{
	VulkanAllocator::VulkanAllocator() = default;
	VulkanAllocator::~VulkanAllocator() = default;

	void VulkanAllocator::Initialize(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t apiVersion)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN MEMORY ALLOCATOR -------");
		
		if (instance == VK_NULL_HANDLE || physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Failed to create allocator either instance, physical device or device is null");

			return;
		}

		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.instance = instance;
		allocatorCreateInfo.physicalDevice = physicalDevice;
		allocatorCreateInfo.device = device;
		allocatorCreateInfo.vulkanApiVersion = apiVersion;

		if (Utilities::VulkanUtilities::VKCheck(vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator), "Failed vmaCreateAllocator"))
		{
			m_Allocator = VK_NULL_HANDLE;

			return;
		}

		IG_CORE_INFO("------- VULKAN MEMORY ALLOCATOR INITIALIZED -------");
	}

	void VulkanAllocator::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN MEMORY ALLOCATOR -------");

		if (m_Allocator == VK_NULL_HANDLE)
		{
			return;
		}

		vmaDestroyAllocator(m_Allocator);
		m_Allocator = VK_NULL_HANDLE;

		IG_CORE_INFO("------- VULKAN MEMORY ALLOCATOR SHUTDOWN COMPLETE -------");
	}
}