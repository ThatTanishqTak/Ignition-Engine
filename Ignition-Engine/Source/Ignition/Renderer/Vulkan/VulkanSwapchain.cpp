#include "Ignition/Renderer/Vulkan/VulkanSwapchain.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <algorithm>

namespace
{
	VkSurfaceFormatKHR ChooseSurfaceFormat(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		uint32_t formatCount = 0;
		Ignition::Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr), "Failed vkGetPhysicalDeviceSurfaceFormatsKHR");

		std::vector<VkSurfaceFormatKHR> formats(formatCount);
		Ignition::Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, formats.data()), "Failed vkGetPhysicalDeviceSurfaceFormatsKHR");

		if (formats.empty())
		{
			IG_CORE_ERROR("No surface formats available");

			return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
		}

		for (const VkSurfaceFormatKHR& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
			{
				return format;
			}
		}

		IG_CORE_WARN("Preferred surface format unavailable, falling back to first reported format");

		return formats[0];
	}

	VkPresentModeKHR ChoosePresentMode(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		uint32_t presentModeCount = 0;
		Ignition::Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr), "Failed vkGetPhysicalDeviceSurfacePresentModesKHR");

		std::vector<VkPresentModeKHR> presentModes(presentModeCount);
		Ignition::Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()), "Failed vkGetPhysicalDeviceSurfacePresentModesKHR");

		for (VkPresentModeKHR presentMode : presentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				IG_CORE_TRACE("Present Mode: Mailbox");

				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
		}

		IG_CORE_TRACE("Present Mode: FIFO");

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
	{
		if (capabilities.currentExtent.width != UINT32_MAX)
		{
			return capabilities.currentExtent;
		}

		VkExtent2D extent{};
		extent.width = std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		extent.height = std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

		return extent;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

namespace Ignition
{
	VulkanSwapchain::VulkanSwapchain() = default;
	VulkanSwapchain::~VulkanSwapchain() = default;

	void VulkanSwapchain::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t graphicsQueueFamily, uint32_t presentQueueFamily, uint32_t width, uint32_t height)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN SWAPCHAIN -------");

		if (physicalDevice == VK_NULL_HANDLE || device == VK_NULL_HANDLE || surface == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Cannot create swapchain from null physical device, device or surface");

			return;
		}

		m_PhysicalDevice = physicalDevice;
		m_Device = device;
		m_Surface = surface;
		m_GraphicsQueueFamily = graphicsQueueFamily;
		m_PresentQueueFamily = presentQueueFamily;

		CreateSwapchain(width, height);
		CreateImageViews();
		CreateSemaphores();

		if (!IsValid())
		{
			return;
		}

		IG_CORE_INFO("------- VULKAN SWAPCHAIN INITIALIZED -------");
	}

	void VulkanSwapchain::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN SWAPCHAIN -------");

		if (m_Device != VK_NULL_HANDLE)
		{
			Utilities::VulkanUtilities::VKCheck(vkDeviceWaitIdle(m_Device), "Failed vkDeviceWaitIdle");
		}

		DestroySemaphores();
		DestroyImageViews();
		DestroySwapchain();

		m_PhysicalDevice = VK_NULL_HANDLE;
		m_Device = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
		m_GraphicsQueueFamily = UINT32_MAX;
		m_PresentQueueFamily = UINT32_MAX;

		IG_CORE_INFO("------- VULKAN SWAPCHAIN SHUTDOWN COMPLETE -------");
	}

	void VulkanSwapchain::Recreate(uint32_t width, uint32_t height)
	{
		IG_CORE_TRACE("Recreating Swapchain: {}x{}", width, height);

		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		Utilities::VulkanUtilities::VKCheck(vkDeviceWaitIdle(m_Device), "Failed vkDeviceWaitIdle");

		DestroySemaphores();
		DestroyImageViews();

		CreateSwapchain(width, height);

		if (m_Swapchain == VK_NULL_HANDLE)
		{
			return;
		}

		CreateImageViews();
		CreateSemaphores();

		IG_CORE_TRACE("Swapchain Recreated");
	}

	void VulkanSwapchain::CreateSwapchain(uint32_t width, uint32_t height)
	{
		IG_CORE_TRACE("Creating Swapchain");

		VkSurfaceCapabilitiesKHR capabilities{};
		if (Utilities::VulkanUtilities::VKCheck(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_PhysicalDevice, m_Surface, &capabilities), "Failed vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
		{
			DestroySwapchain();

			return;
		}

		const VkExtent2D extent = ChooseExtent(capabilities, width, height);

		if (extent.width == 0 || extent.height == 0)
		{
			IG_CORE_TRACE("Window minimized");

			DestroySwapchain();

			m_Extent = extent;

			return;
		}

		const VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(m_PhysicalDevice, m_Surface);

		if (surfaceFormat.format == VK_FORMAT_UNDEFINED)
		{
			DestroySwapchain();

			return;
		}

		const VkPresentModeKHR presentMode = ChoosePresentMode(m_PhysicalDevice, m_Surface);

		uint32_t imageCount = capabilities.minImageCount + 1;
		if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount)
		{
			imageCount = capabilities.maxImageCount;
		}

		VkSwapchainCreateInfoKHR swapchainCreateInfo{};
		swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapchainCreateInfo.surface = m_Surface;
		swapchainCreateInfo.minImageCount = imageCount;
		swapchainCreateInfo.imageFormat = surfaceFormat.format;
		swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
		swapchainCreateInfo.imageExtent = extent;
		swapchainCreateInfo.imageArrayLayers = 1;
		swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		swapchainCreateInfo.preTransform = capabilities.currentTransform;
		swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swapchainCreateInfo.presentMode = presentMode;
		swapchainCreateInfo.clipped = VK_TRUE;
		swapchainCreateInfo.oldSwapchain = m_Swapchain;

		const uint32_t queueFamilies[] = { m_GraphicsQueueFamily, m_PresentQueueFamily };

		if (m_GraphicsQueueFamily != m_PresentQueueFamily)
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swapchainCreateInfo.queueFamilyIndexCount = 2;
			swapchainCreateInfo.pQueueFamilyIndices = queueFamilies;
		}
		else
		{
			swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		}

		VkSwapchainKHR newSwapchain = VK_NULL_HANDLE;

		const bool createFailed = Utilities::VulkanUtilities::VKCheck(vkCreateSwapchainKHR(m_Device, &swapchainCreateInfo, nullptr, &newSwapchain), "Failed vkCreateSwapchainKHR");
		DestroySwapchain();

		if (createFailed)
		{
			return;
		}

		m_Swapchain = newSwapchain;
		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;

		uint32_t actualImageCount = 0;
		Utilities::VulkanUtilities::VKCheck(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &actualImageCount, nullptr), "Failed vkGetSwapchainImagesKHR");

		m_Images.resize(actualImageCount);
		Utilities::VulkanUtilities::VKCheck(vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &actualImageCount, m_Images.data()), "Failed vkGetSwapchainImagesKHR");

		IG_CORE_TRACE("Swapchain Created: {}x{}, {} images", m_Extent.width, m_Extent.height, actualImageCount);
	}

	void VulkanSwapchain::CreateImageViews()
	{
		IG_CORE_TRACE("Creating Image Views");

		m_ImageViews.resize(m_Images.size(), VK_NULL_HANDLE);

		for (size_t i = 0; i < m_Images.size(); ++i)
		{
			VkImageViewCreateInfo imageViewCreateInfo{};
			imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			imageViewCreateInfo.image = m_Images[i];
			imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			imageViewCreateInfo.format = m_ImageFormat;
			imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
			imageViewCreateInfo.subresourceRange.levelCount = 1;
			imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
			imageViewCreateInfo.subresourceRange.layerCount = 1;

			if (Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &imageViewCreateInfo, nullptr, &m_ImageViews[i]), "Failed vkCreateImageView"))
			{
				DestroyImageViews();
				DestroySwapchain();

				return;
			}
		}

		IG_CORE_TRACE("Created {} Swapchain Image Views", m_ImageViews.size());
	}

	void VulkanSwapchain::CreateSemaphores()
	{
		IG_CORE_TRACE("Creating Semaphores");

		m_RenderFinishedSemaphores.resize(m_Images.size(), VK_NULL_HANDLE);

		VkSemaphoreCreateInfo semaphoreCreateInfo{};
		semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i)
		{
			if (Utilities::VulkanUtilities::VKCheck(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed vkCreateSemaphore"))
			{
				DestroySemaphores();
				DestroyImageViews();
				DestroySwapchain();

				return;
			}
		}

		IG_CORE_TRACE("Created {} Render Finished Semaphores", m_RenderFinishedSemaphores.size());
	}

	void VulkanSwapchain::DestroySwapchain()
	{
		IG_CORE_TRACE("Destroying Swapchain");

		if (m_Swapchain == VK_NULL_HANDLE)
		{
			return;
		}

		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		m_Swapchain = VK_NULL_HANDLE;
		m_Images.clear();

		IG_CORE_TRACE("Swapchain Destroyed");
	}

	void VulkanSwapchain::DestroyImageViews()
	{
		IG_CORE_TRACE("Destroying Image Views");

		for (VkImageView imageView : m_ImageViews)
		{
			if (imageView != VK_NULL_HANDLE)
			{
				vkDestroyImageView(m_Device, imageView, nullptr);
			}
		}

		m_ImageViews.clear();

		IG_CORE_TRACE("Image Views Destroyed");
	}

	void VulkanSwapchain::DestroySemaphores()
	{
		IG_CORE_TRACE("Destroying Semaphores");

		for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
		{
			if (semaphore != VK_NULL_HANDLE)
			{
				vkDestroySemaphore(m_Device, semaphore, nullptr);
			}
		}

		m_RenderFinishedSemaphores.clear();

		IG_CORE_TRACE("Semaphores Destroyed");
	}
}