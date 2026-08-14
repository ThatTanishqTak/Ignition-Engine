#include "Ignition/Renderer/Vulkan/VulkanDevice.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <cstring>
#include <vector>

namespace
{
	const char* s_RequiredDeviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

	struct QueueFamilyIndices
	{
		uint32_t Graphics = UINT32_MAX;
		uint32_t Present = UINT32_MAX;

		bool IsComplete() const { return Graphics != UINT32_MAX && Present != UINT32_MAX; }
	};

	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		QueueFamilyIndices indices{};

		uint32_t familyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, nullptr);

		std::vector<VkQueueFamilyProperties> families(familyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &familyCount, families.data());

		for (uint32_t i = 0; i < familyCount; ++i)
		{
			if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				if (indices.Graphics == UINT32_MAX)
				{
					indices.Graphics = i;
				}
			}

			VkBool32 presentSupported = VK_FALSE;
			VK_CHECK(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupported));

			if (presentSupported == VK_TRUE && indices.Present == UINT32_MAX)
			{
				indices.Present = i;
			}

			if ((families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupported == VK_TRUE)
			{
				indices.Graphics = i;
				indices.Present = i;

				break;
			}
		}

		return indices;
	}

	bool CheckDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
	{
		uint32_t extensionCount = 0;
		VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr));

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		VK_CHECK(vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data()));

		for (const char* requiredDeviceExtension : s_RequiredDeviceExtensions)
		{
			bool found = false;

			for (const VkExtensionProperties& availableExtension : availableExtensions)
			{
				if (std::strcmp(availableExtension.extensionName, requiredDeviceExtension) == 0)
				{
					found = true;

					break;
				}
			}

			if (!found)
			{
				return false;
			}
		}

		return true;
	}

	bool HasAdequateSurfaceSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		uint32_t formatCount = 0;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr));

		uint32_t presentModeCount = 0;
		VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));

		return formatCount > 0 && presentModeCount > 0;
	}

	bool HasRequiredFeatures(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
		physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;

		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &physicalDeviceVulkan13Features;

		vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

		return physicalDeviceVulkan13Features.dynamicRendering == VK_TRUE && physicalDeviceVulkan13Features.synchronization2 == VK_TRUE;
	}

	uint32_t ScoreDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		if (!FindQueueFamilies(physicalDevice, surface).IsComplete() || !CheckDeviceExtensionSupport(physicalDevice) || !HasAdequateSurfaceSupport(physicalDevice, surface) || !HasRequiredFeatures(physicalDevice))
		{
			return 0;
		}

		VkPhysicalDeviceProperties physicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			return 1000;
		}

		if (physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
		{
			return 100;
		}

		return 10;
	}
}

namespace Ignition
{
	VulkanDevice::VulkanDevice() = default;
	VulkanDevice::~VulkanDevice() = default;

	void VulkanDevice::Initialize(VkInstance instance, VkSurfaceKHR surface)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN DEVICE -------");

		if (instance == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Cannot create device from a null instance");

			return;
		}

		if (surface == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Cannot create device from a null surface");

			return;
		}

		m_Surface = surface;

		PickPhysicalDevice(instance, surface);
		CreateLogicalDevice();

		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		IG_CORE_INFO("------- VULKAN DEVICE INITIALIZED -------");
	}

	void VulkanDevice::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN DEVICE -------");

		DestroyLogicalDevice();

		IG_CORE_INFO("------- VULKAN DEVICE SHUTDOWN COMPLETE -------");
	}

	void VulkanDevice::PickPhysicalDevice(VkInstance instance, VkSurfaceKHR surface)
	{
		IG_CORE_TRACE("Selecting Physical Device");

		uint32_t deviceCount = 0;
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr));

		if (deviceCount == 0)
		{
			IG_CORE_CRITICAL("No physical devices with Vulkan support");

			return;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		VK_CHECK(vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()));

		uint32_t bestScore = 0;
		for (VkPhysicalDevice device : devices)
		{
			const uint32_t score = ScoreDevice(device, surface);
			if (score > bestScore)
			{
				bestScore = score;
				m_PhysicalDevice = device;
			}
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("No suitable physical device found");

			return;
		}

		VkPhysicalDeviceProperties physicalDeviceProperties{};
		vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

		IG_CORE_TRACE("Selected GPU: {}", physicalDeviceProperties.deviceName);

		const QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice, surface);
		m_GraphicsQueueFamily = indices.Graphics;
		m_PresentQueueFamily = indices.Present;

		if (m_GraphicsQueueFamily == m_PresentQueueFamily)
		{
			IG_CORE_TRACE("Graphics And Present Share Queue Family: {}", m_GraphicsQueueFamily);
		}
		else
		{
			IG_CORE_TRACE("Graphics Queue Family {}, Present Queue Family: {}", m_GraphicsQueueFamily, m_PresentQueueFamily);
		}
	}

	void VulkanDevice::CreateLogicalDevice()
	{
		IG_CORE_TRACE("Creating Logical Device");

		if (m_PhysicalDevice == VK_NULL_HANDLE || m_GraphicsQueueFamily == UINT32_MAX || m_PresentQueueFamily == UINT32_MAX)
		{
			return;
		}

		const float queuePriority = 1.0f;

		std::vector<uint32_t> uniqueQueueFamilies;
		uniqueQueueFamilies.push_back(m_GraphicsQueueFamily);

		if (m_PresentQueueFamily != m_GraphicsQueueFamily)
		{
			uniqueQueueFamilies.push_back(m_PresentQueueFamily);
		}

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		for (uint32_t family : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = family;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;

			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures physicalDeviceFeatures{};
		VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features{};
		physicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		physicalDeviceVulkan11Features.shaderDrawParameters = VK_TRUE;

		VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
		physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		physicalDeviceVulkan13Features.dynamicRendering = VK_TRUE;
		physicalDeviceVulkan13Features.synchronization2 = VK_TRUE;
		physicalDeviceVulkan13Features.pNext = &physicalDeviceVulkan11Features;

		VkDeviceCreateInfo deviceCreateInfo{};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = &physicalDeviceVulkan13Features;
		deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
		deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(std::size(s_RequiredDeviceExtensions));
		deviceCreateInfo.ppEnabledExtensionNames = s_RequiredDeviceExtensions;

		for (const char* extension : s_RequiredDeviceExtensions)
		{
			IG_CORE_TRACE("Enabling Device Extension: {}", extension);
		}

		if (!VK_CHECK(vkCreateDevice(m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device)))
		{
			return;
		}

		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		vkGetDeviceQueue(m_Device, m_GraphicsQueueFamily, 0, &m_GraphicsQueue);
		vkGetDeviceQueue(m_Device, m_PresentQueueFamily, 0, &m_PresentQueue);

		IG_CORE_TRACE("Logical Device Created");
	}

	void VulkanDevice::DestroyLogicalDevice()
	{
		IG_CORE_TRACE("Destroying Logical Device");

		if (m_Device == VK_NULL_HANDLE)
		{
			return;
		}

		VK_CHECK(vkDeviceWaitIdle(m_Device));
		vkDestroyDevice(m_Device, nullptr);

		m_Device = VK_NULL_HANDLE;
		m_GraphicsQueue = VK_NULL_HANDLE;
		m_PresentQueue = VK_NULL_HANDLE;
		m_PhysicalDevice = VK_NULL_HANDLE;
		m_Surface = VK_NULL_HANDLE;
		m_GraphicsQueueFamily = UINT32_MAX;
		m_PresentQueueFamily = UINT32_MAX;

		IG_CORE_TRACE("Logical Device Destroyed");
	}
}