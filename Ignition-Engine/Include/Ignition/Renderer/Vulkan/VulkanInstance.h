#pragma once

#include <vulkan/vulkan.h>

#include <memory>

namespace Ignition
{
	class VulkanDebugMessenger;

	class VulkanInstance
	{
	public:
		VulkanInstance();
		~VulkanInstance();

		VulkanInstance(const VulkanInstance&) = delete;
		VulkanInstance& operator=(const VulkanInstance&) = delete;

		void Initialize();
		void Shutdown();

		VkInstance GetInstance() const { return m_Instance; }
		uint32_t GetAPIVersion() const { return m_APIVersion; }
		bool IsValidationEnabled() const { return m_ValidationEnabled; }

	private:
		void CreateInstance();
		void DestroyInstance();

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		std::unique_ptr<VulkanDebugMessenger> m_DebugMessenger;
		uint32_t m_APIVersion = 0;
		bool m_ValidationEnabled = false;
	};
}