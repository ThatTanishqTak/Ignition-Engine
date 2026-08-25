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

		const std::vector<VkQueueFamilyProperties> families = Ignition::Utilities::Enumerate<VkQueueFamilyProperties>(vkGetPhysicalDeviceQueueFamilyProperties, physicalDevice);

		for (uint32_t i = 0; i < static_cast<uint32_t>(families.size()); ++i)
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
		VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
		physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;

		VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
		physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		physicalDeviceVulkan13Features.pNext = &physicalDeviceVulkan12Features;

		VkPhysicalDeviceFeatures2 features2{};
		features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		features2.pNext = &physicalDeviceVulkan13Features;

		vkGetPhysicalDeviceFeatures2(physicalDevice, &features2);

		return physicalDeviceVulkan13Features.dynamicRendering == VK_TRUE && physicalDeviceVulkan13Features.synchronization2 == VK_TRUE && physicalDeviceVulkan12Features.descriptorIndexing == VK_TRUE && physicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing == VK_TRUE && physicalDeviceVulkan12Features.descriptorBindingSampledImageUpdateAfterBind == VK_TRUE && physicalDeviceVulkan12Features.descriptorBindingPartiallyBound == VK_TRUE && physicalDeviceVulkan12Features.runtimeDescriptorArray == VK_TRUE;
	}

	const char* RejectDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
	{
		if (!FindQueueFamilies(physicalDevice, surface).IsComplete())
		{
			return "no queue family with both graphics and present support";
		}

		if (!CheckDeviceExtensionSupport(physicalDevice))
		{
			return "VK_KHR_swapchain unsupported";
		}

		if (!HasAdequateSurfaceSupport(physicalDevice, surface))
		{
			return "no surface formats or present modes for this window";
		}

		if (!HasRequiredFeatures(physicalDevice))
		{
			return "dynamicRendering, synchronization2 or descriptor indexing unavailable";
		}

		return nullptr;
	}

	uint32_t ScoreDevice(VkPhysicalDevice physicalDevice)
	{
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

	const char* DeviceTypeName(VkPhysicalDeviceType type)
	{
		switch (type)
		{
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				return "discrete";
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				return "integrated";
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				return "virtual";
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				return "cpu";
			default:
				return "other";
		}
	}

	// Device-local heap total - the number Appendix B's lattice budget actually has to fit inside, and which nothing printed until now
	double DeviceLocalMegabytes(VkPhysicalDevice physicalDevice)
	{
		VkPhysicalDeviceMemoryProperties memoryProperties{};
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

		VkDeviceSize total = 0;

		for (uint32_t heap = 0; heap < memoryProperties.memoryHeapCount; ++heap)
		{
			if ((memoryProperties.memoryHeaps[heap].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) != 0)
			{
				total += memoryProperties.memoryHeaps[heap].size;
			}
		}

		return static_cast<double>(total) / (1024.0 * 1024.0);
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

		const std::vector<VkPhysicalDevice> devices = Utilities::Enumerate<VkPhysicalDevice>(vkEnumeratePhysicalDevices, instance);

		if (devices.empty())
		{
			IG_CORE_CRITICAL("No physical devices with Vulkan support");

			return;
		}

		IG_CORE_TRACE("Enumerated {} physical device(s)", devices.size());

		uint32_t bestScore = 0;

		for (VkPhysicalDevice device : devices)
		{
			VkPhysicalDeviceProperties candidateProperties{};
			vkGetPhysicalDeviceProperties(device, &candidateProperties);

			const char* const type = DeviceTypeName(candidateProperties.deviceType);
			const double megabytes = DeviceLocalMegabytes(device);

			if (const char* rejection = RejectDevice(device, surface))
			{
				IG_CORE_TRACE("Rejected GPU: {} ({}, {:.0f} MB) - {}", candidateProperties.deviceName, type, megabytes, rejection);

				continue;
			}

			const uint32_t score = ScoreDevice(device);

			IG_CORE_TRACE("Candidate GPU: {} ({}, {:.0f} MB, score {})", candidateProperties.deviceName, type, megabytes, score);

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

		m_TimestampPeriod = physicalDeviceProperties.limits.timestampPeriod;

		IG_CORE_TRACE("Selected GPU: {} ({}, {:.0f} MB device-local)", physicalDeviceProperties.deviceName, DeviceTypeName(physicalDeviceProperties.deviceType), DeviceLocalMegabytes(m_PhysicalDevice));

		if (physicalDeviceProperties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
		{
			IG_CORE_WARN("No discrete GPU selected. Wind tunnel performance and allocations will not be representative - check the candidate lines above and this executable's system GPU preference");
		}

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
		physicalDeviceFeatures.fragmentStoresAndAtomics = VK_TRUE; // the volume ray march reads the lattice from a fragment shader
		VkPhysicalDeviceVulkan11Features physicalDeviceVulkan11Features{};
		physicalDeviceVulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
		physicalDeviceVulkan11Features.shaderDrawParameters = VK_TRUE;

		// The UI's bindless texture table indexes one variable-count sampler array, updated after bind and only partially populated
		VkPhysicalDeviceVulkan12Features physicalDeviceVulkan12Features{};
		physicalDeviceVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		physicalDeviceVulkan12Features.descriptorIndexing = VK_TRUE;
		physicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
		physicalDeviceVulkan12Features.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;
		physicalDeviceVulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
		physicalDeviceVulkan12Features.runtimeDescriptorArray = VK_TRUE;
		physicalDeviceVulkan12Features.pNext = &physicalDeviceVulkan11Features;

		VkPhysicalDeviceVulkan13Features physicalDeviceVulkan13Features{};
		physicalDeviceVulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		physicalDeviceVulkan13Features.dynamicRendering = VK_TRUE;
		physicalDeviceVulkan13Features.synchronization2 = VK_TRUE;
		physicalDeviceVulkan13Features.pNext = &physicalDeviceVulkan12Features;

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