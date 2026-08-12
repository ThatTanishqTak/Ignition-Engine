#include "Ignition/Renderer/Vulkan/VulkanImage.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <vk_mem_alloc.h>

namespace Ignition
{
	VulkanImage::VulkanImage() = default;
	VulkanImage::~VulkanImage() = default;

	void VulkanImage::Initialize(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN IMAGE -------");

		m_Device = device;
		m_Allocator = allocator;
		m_Format = format;
		m_Extent = { width, height };

		if (device == VK_NULL_HANDLE || allocator == VK_NULL_HANDLE || width == 0 || height == 0)
		{
			IG_CORE_ERROR("Vulkan image initialization aborted: invalid device, allocator or extent");

			return;
		}

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.mipLevels = 1;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = usage;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocationCreateInfo{};
		allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

		if (Utilities::VulkanUtilities::VKCheck(vmaCreateImage(allocator, &imageCreateInfo, &allocationCreateInfo, &m_Image, &m_Allocation, nullptr), "Failed vmaCreateImage"))
		{
			m_Image = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;

			return;
		}

		VkImageViewCreateInfo viewCreateInfo{};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.image = m_Image;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange.aspectMask = aspectMask;
		viewCreateInfo.subresourceRange.baseMipLevel = 0;
		viewCreateInfo.subresourceRange.levelCount = 1;
		viewCreateInfo.subresourceRange.baseArrayLayer = 0;
		viewCreateInfo.subresourceRange.layerCount = 1;

		if (Utilities::VulkanUtilities::VKCheck(vkCreateImageView(m_Device, &viewCreateInfo, nullptr, &m_ImageView), "Failed vkCreateImageView"))
		{
			m_ImageView = VK_NULL_HANDLE;

			vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;

			return;
		}

		IG_CORE_INFO("------- VULKAN IMAGE INITIALIZED -------");
	}

	void VulkanImage::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN IMAGE -------");

		if (m_ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_Device, m_ImageView, nullptr);
			m_ImageView = VK_NULL_HANDLE;
		}

		if (m_Image != VK_NULL_HANDLE)
		{
			vmaDestroyImage(m_Allocator, m_Image, m_Allocation);
			m_Image = VK_NULL_HANDLE;
			m_Allocation = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;
		m_Allocator = VK_NULL_HANDLE;
		m_Format = VK_FORMAT_UNDEFINED;
		m_Extent = { 0, 0 };

		IG_CORE_INFO("------- VULKAN IMAGE SHUTDOWN COMPLETE -------");
	}
}