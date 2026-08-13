#pragma once

#include <vulkan/vulkan.h>

namespace Ignition
{
	class VulkanDebugMessenger
	{
	public:
		VulkanDebugMessenger();
		~VulkanDebugMessenger();

		VulkanDebugMessenger(const VulkanDebugMessenger&) = delete;
		VulkanDebugMessenger& operator=(const VulkanDebugMessenger&) = delete;

		void Initialize(VkInstance instance);
		void Shutdown();

		VkDebugUtilsMessengerEXT GetDebugMessenger() const { return m_DebugMessenger; }

		static bool IsValidationEnabled();
		static const char* GetValidationLayerName();
		static void PopulateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
	};
}