#include "Ignition/Renderer/Vulkan/VulkanInstance.h"

#include "Ignition/Renderer/Vulkan/VulkanDebugMessenger.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <SDL3/SDL_vulkan.h>

#include <vector>

namespace Ignition
{
	VulkanInstance::VulkanInstance() = default;
	VulkanInstance::~VulkanInstance() = default;

	void VulkanInstance::Initialize()
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN INSTANCE -------");

		CreateInstance();

		m_DebugMessenger = std::make_unique<VulkanDebugMessenger>();
		m_DebugMessenger->Initialize(m_Instance);

		IG_CORE_INFO("------- VULKAN INSTANCE INITIALIZED -------");
	}

	void VulkanInstance::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN INSTANCE -------");

		if (m_DebugMessenger)
		{
			m_DebugMessenger->Shutdown();
			m_DebugMessenger.reset();
		}

		DestroyInstance();

		IG_CORE_INFO("------- VULKAN INSTANCE SHUTDOWN COMPLETE -------");
	}

	void VulkanInstance::CreateInstance()
	{
		IG_CORE_TRACE("Creating Vulkan Instance");

		VkApplicationInfo applicationInfo{};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "Ignition-Sandbox";
		applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
		applicationInfo.pEngineName = "Ignition";
		applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);

		uint32_t instanceVersion = VK_API_VERSION_1_0;
		if (vkEnumerateInstanceVersion(&instanceVersion) != VK_SUCCESS)
		{
			instanceVersion = VK_API_VERSION_1_0;
		}

		if (instanceVersion < VK_API_VERSION_1_3)
		{
			IG_CORE_CRITICAL("Vulkan 1.3 or newer required, found {}.{}", VK_API_VERSION_MAJOR(instanceVersion), VK_API_VERSION_MINOR(instanceVersion));

			return;
		}

		m_APIVersion = instanceVersion >= VK_API_VERSION_1_4 ? VK_API_VERSION_1_4 : VK_API_VERSION_1_3;
		applicationInfo.apiVersion = m_APIVersion;

		IG_CORE_TRACE("Using Vulkan API {}.{}", VK_API_VERSION_MAJOR(m_APIVersion), VK_API_VERSION_MINOR(m_APIVersion));

		uint32_t sdlExtensionCount = 0;
		const char* const* sdlExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
		if (sdlExtensions == nullptr)
		{
			IG_CORE_CRITICAL("Failed SDL_Vulkan_GetInstanceExtensions: {}", SDL_GetError());

			return;
		}

		std::vector<const char*> extensions(sdlExtensions, sdlExtensions + sdlExtensionCount);

		m_ValidationEnabled = VulkanDebugMessenger::IsValidationEnabled();

		std::vector<const char*> layers;
		if (m_ValidationEnabled)
		{
			layers.push_back(VulkanDebugMessenger::GetValidationLayerName());
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

			for (const char* layer : layers)
			{
				IG_CORE_TRACE("Enabling Instance Layer: {}", layer);
			}
		}

		for (const char* extension : extensions)
		{
			IG_CORE_TRACE("Enabling Instance Extension: {}", extension);
		}

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		VulkanDebugMessenger::PopulateCreateInfo(debugCreateInfo);

		VkInstanceCreateInfo instanceCreateInfo{};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
		instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		instanceCreateInfo.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
		instanceCreateInfo.pNext = m_ValidationEnabled ? &debugCreateInfo : nullptr;

		if (Utilities::VulkanUtilities::VKCheck(vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance), "Failed vkCreateInstance"))
		{
			return;
		}

		IG_CORE_TRACE("Vulkan Instance Created");
	}

	void VulkanInstance::DestroyInstance()
	{
		IG_CORE_TRACE("Destroying Vulkan Instance");

		if (m_Instance == VK_NULL_HANDLE)
		{
			return;
		}

		vkDestroyInstance(m_Instance, nullptr);
		m_Instance = VK_NULL_HANDLE;

		IG_CORE_TRACE("Vulkan Instance Destroyed");
	}
}