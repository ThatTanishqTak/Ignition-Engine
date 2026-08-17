#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Ignition
{
	class VulkanDevice
	{
	public:
		VulkanDevice();
		~VulkanDevice();

		VulkanDevice(const VulkanDevice&) = delete;
		VulkanDevice& operator=(const VulkanDevice&) = delete;

		void Initialize(VkInstance instance, VkSurfaceKHR surface);
		void Shutdown();

		VkSurfaceKHR GetSurface() const { return m_Surface; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }
		uint32_t GetGraphicsQueueFamily() const { return m_GraphicsQueueFamily; }
		uint32_t GetPresentQueueFamily() const { return m_PresentQueueFamily; }

		// Nanoseconds per timestamp tick; zero when the device reports no timestamp support
		float GetTimestampPeriod() const { return m_TimestampPeriod; }

	private:
		void PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface);
		void CreateLogicalDevice();
		void DestroyLogicalDevice();

	private:
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;
		uint32_t m_GraphicsQueueFamily = UINT32_MAX;
		uint32_t m_PresentQueueFamily = UINT32_MAX;
		float m_TimestampPeriod = 0.0f;
	};
}