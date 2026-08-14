#include "Ignition/Renderer/Vulkan/VulkanDebugMessenger.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <cstring>
#include <vector>

namespace
{
#ifdef NDEBUG
	constexpr bool s_EnableValidation = false;
#else
	constexpr bool s_EnableValidation = true;
#endif

	constexpr const char* s_ValidationLayerName = "VK_LAYER_KHRONOS_validation";

	bool IsValidationLayerAvailable()
	{
		uint32_t layerCount = 0;
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, nullptr));

		std::vector<VkLayerProperties> availableLayers(layerCount);
		VK_CHECK(vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data()));

		for (const VkLayerProperties& layer : availableLayers)
		{
			if (std::strcmp(layer.layerName, s_ValidationLayerName) == 0)
			{
				return true;
			}
		}

		return false;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData)
	{
		(void)userData;
		(void)type;

		if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		{
			IG_CORE_ERROR("[VULKAN]: {}", callbackData->pMessage);
		}
		else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		{
			IG_CORE_WARN("[VULKAN]: {}", callbackData->pMessage);
		}
		else
		{
			IG_CORE_TRACE("[VULKAN]: {}", callbackData->pMessage);
		}

		return VK_FALSE;
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

namespace Ignition
{
	VulkanDebugMessenger::VulkanDebugMessenger() = default;
	VulkanDebugMessenger::~VulkanDebugMessenger() = default;

	bool VulkanDebugMessenger::IsValidationEnabled()
	{
		static const bool s_Enabled = []()
		{
			if (!s_EnableValidation)
			{
				return false;
			}

			if (!IsValidationLayerAvailable())
			{
				IG_CORE_WARN("Validation layer requested but not available");

				return false;
			}

			return true;
		}();

		return s_Enabled;
	}

	const char* VulkanDebugMessenger::GetValidationLayerName()
	{
		return s_ValidationLayerName;
	}

	void VulkanDebugMessenger::PopulateCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		IG_CORE_TRACE("Populating Create Info");

		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = DebugCallback;

		IG_CORE_TRACE("Create Info Populated");
	}

	void VulkanDebugMessenger::Initialize(VkInstance instance)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN DEBUG MESSENGER -------");

		if (!IsValidationEnabled() || instance == VK_NULL_HANDLE)
		{
			return;
		}

		m_Instance = instance;

		auto createFunction = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT"));

		if (createFunction == nullptr)
		{
			IG_CORE_WARN("vkCreateDebugUtilsMessengerEXT unavailable, debug messenger disabled");

			return;
		}

		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		PopulateCreateInfo(createInfo);

		if (!VK_CHECK(createFunction(m_Instance, &createInfo, nullptr, &m_DebugMessenger)))
		{
			return;
		}

		IG_CORE_INFO("------- VULKAN DEBUG MESSENGER INITIALIZED -------");
	}

	void VulkanDebugMessenger::Shutdown()
	{
		if (m_DebugMessenger == VK_NULL_HANDLE || m_Instance == VK_NULL_HANDLE)
		{
			return;
		}

		IG_CORE_INFO("------- SHUTTING DOWN VULKAN DEBUG MESSENGER -------");

		auto destroyFunction = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT"));

		if (destroyFunction != nullptr)
		{
			destroyFunction(m_Instance, m_DebugMessenger, nullptr);
		}

		m_DebugMessenger = VK_NULL_HANDLE;
		m_Instance = VK_NULL_HANDLE;

		IG_CORE_INFO("------- VULKAN DEBUG MESSENGER SHUTDOWN COMPLETE -------");
	}
}