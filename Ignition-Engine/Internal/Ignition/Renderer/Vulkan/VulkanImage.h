#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaAllocation)

namespace Ignition
{
	class VulkanImage
	{
	public:
		VulkanImage();
		~VulkanImage();

		VulkanImage(const VulkanImage&) = delete;
		VulkanImage& operator=(const VulkanImage&) = delete;

		void Initialize(VkDevice device, VmaAllocator allocator, uint32_t width, uint32_t height, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspectMask);
		void Shutdown();

		bool IsValid() const { return m_ImageView != VK_NULL_HANDLE; }

		VkImage GetImage() const { return m_Image; }
		VkImageView GetImageView() const { return m_ImageView; }
		VkFormat GetFormat() const { return m_Format; }
		VkExtent2D GetExtent() const { return m_Extent; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
		VkImage m_Image = VK_NULL_HANDLE;
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkFormat m_Format = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent{ 0, 0 };
	};
}