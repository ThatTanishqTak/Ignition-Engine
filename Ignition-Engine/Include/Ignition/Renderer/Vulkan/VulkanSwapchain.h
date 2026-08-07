#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Ignition
{
	class VulkanSwapchain
	{
	public:
		VulkanSwapchain();
		~VulkanSwapchain();

		VulkanSwapchain(const VulkanSwapchain&) = delete;
		VulkanSwapchain& operator=(const VulkanSwapchain&) = delete;

		void Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkSurfaceKHR surface, uint32_t graphicsQueueFamily, uint32_t presentQueueFamily, uint32_t width, uint32_t height);
		void Shutdown();

		void Recreate(uint32_t width, uint32_t height);

		VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
		VkFormat GetImageFormat() const { return m_ImageFormat; }
		VkExtent2D GetExtent() const { return m_Extent; }

		uint32_t GetImageCount() const { return static_cast<uint32_t>(m_Images.size()); }
		VkImage GetImage(uint32_t index) const { return m_Images[index]; }
		VkImageView GetImageView(uint32_t index) const { return m_ImageViews[index]; }
		VkSemaphore GetRenderFinishedSemaphore(uint32_t imageIndex) const { return m_RenderFinishedSemaphores[imageIndex]; }

		bool IsValid() const { return m_Swapchain != VK_NULL_HANDLE; }

	private:
		void CreateSwapchain(uint32_t width, uint32_t height);
		void CreateImageViews();
		void CreateSemaphores();

		void DestroySwapchain();
		void DestroyImageViews();
		void DestroySemaphores();

	private:
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		uint32_t m_GraphicsQueueFamily = UINT32_MAX;
		uint32_t m_PresentQueueFamily = UINT32_MAX;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		VkFormat m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent{ 0, 0 };

		std::vector<VkImage> m_Images;
		std::vector<VkImageView> m_ImageViews;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
	};
}