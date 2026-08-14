#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

namespace Ignition
{
	VulkanFrameContext::VulkanFrameContext() = default;
	VulkanFrameContext::~VulkanFrameContext() = default;

	void VulkanFrameContext::Initialize(VkDevice device, uint32_t graphicsQueueFamily)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN FRAME CONTEXT -------");

		if (device == VK_NULL_HANDLE || graphicsQueueFamily == UINT32_MAX)
		{
			IG_CORE_CRITICAL("Cannot create frame context from a null device or invalid queue family");

			return;
		}

		m_Device = device;

		CreateCommandPool(graphicsQueueFamily);

		if (m_CommandPool == VK_NULL_HANDLE)
		{
			return;
		}

		AllocateCommandBuffers();
		CreateSyncObjects();

		if (!IsValid())
		{
			return;
		}

		IG_CORE_INFO("------- VULKAN FRAME CONTEXT INITIALIZED -------");
	}

	bool VulkanFrameContext::IsValid() const
	{
		if (m_CommandPool == VK_NULL_HANDLE || m_CommandBuffers.size() != MaximumFramesInFlight || m_ImageAvailableSemaphores.size() != MaximumFramesInFlight || m_InFlightFences.size() != MaximumFramesInFlight)
		{
			return false;
		}

		for (uint32_t i = 0; i < MaximumFramesInFlight; ++i)
		{
			if (m_CommandBuffers[i] == VK_NULL_HANDLE || m_ImageAvailableSemaphores[i] == VK_NULL_HANDLE || m_InFlightFences[i] == VK_NULL_HANDLE)
			{
				return false;
			}
		}

		return true;
	}

	void VulkanFrameContext::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN FRAME CONTEXT -------");

		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		VK_CHECK(vkDeviceWaitIdle(m_Device));

		for (VkFence fence : m_InFlightFences)
		{
			if (fence != VK_NULL_HANDLE)
			{
				vkDestroyFence(m_Device, fence, nullptr);
			}
		}

		for (VkSemaphore semaphore : m_ImageAvailableSemaphores)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, semaphore, nullptr);
			}
		}

		m_InFlightFences.clear();
		m_ImageAvailableSemaphores.clear();
		m_CommandBuffers.clear();

		if (m_CommandPool != VK_NULL_HANDLE)
		{
			vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
			m_CommandPool = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;

		IG_CORE_INFO("------- VULKAN FRAME CONTEXT SHUTDOWN COMPLETE -------");
	}

	void VulkanFrameContext::CreateCommandPool(uint32_t graphicsQueueFamily)
	{
		IG_CORE_TRACE("Creating Command Pool");

		VkCommandPoolCreateInfo commandPoolCreateInfo{};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamily;

		if (!VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolCreateInfo, nullptr, &m_CommandPool)))
		{
			return;
		}

		IG_CORE_TRACE("Command Pool Created");
	}

	void VulkanFrameContext::AllocateCommandBuffers()
	{
		IG_CORE_TRACE("Allocating Command Buffers");

		m_CommandBuffers.resize(MaximumFramesInFlight, VK_NULL_HANDLE);

		VkCommandBufferAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocateInfo.commandPool = m_CommandPool;
		allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocateInfo.commandBufferCount = MaximumFramesInFlight;

		if (!VK_CHECK(vkAllocateCommandBuffers(m_Device, &allocateInfo, m_CommandBuffers.data())))
		{
			return;
		}

		IG_CORE_TRACE("Allocated {} Command Buffers", MaximumFramesInFlight);
	}

	void VulkanFrameContext::CreateSyncObjects()
	{
		IG_CORE_TRACE("Creating Sync Objects");

		m_ImageAvailableSemaphores.resize(MaximumFramesInFlight, VK_NULL_HANDLE);
		m_InFlightFences.resize(MaximumFramesInFlight, VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceCreateInfo{};
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (uint32_t i = 0; i < MaximumFramesInFlight; ++i)
		{
			if (!VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphores[i])) || !VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_InFlightFences[i])))
			{
				return;
			}
		}

		IG_CORE_TRACE("Created Sync Objects For {} Frames In Flight", MaximumFramesInFlight);
	}
}