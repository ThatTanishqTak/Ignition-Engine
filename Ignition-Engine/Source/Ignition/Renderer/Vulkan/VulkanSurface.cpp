#include "Ignition/Renderer/Vulkan/VulkanSurface.h"

#include "Ignition/Core/Log.h"

#include <SDL3/SDL_vulkan.h>

namespace Ignition
{
	VulkanSurface::VulkanSurface() = default;
	VulkanSurface::~VulkanSurface() = default;
	
	void VulkanSurface::Initialize(VkInstance instance, SDL_Window* nativeWindow)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN SURFACE -------");

		if (instance == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Cannot create surface from a null instance");

			return;
		}

		if (nativeWindow == nullptr)
		{
			IG_CORE_CRITICAL("Cannot create surface from a null window");

			return;
		}

		m_Instance = instance;
		CreateSurface(m_Instance, nativeWindow);

		if (!IsValid())
		{
			return;
		}

		IG_CORE_INFO("------- VULKAN SURFACE INITIALIZED -------");
	}

	void VulkanSurface::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN SURFACE -------");

		DestroySurface();

		IG_CORE_INFO("------- VULKAN SURFACE SHUTDOWN COMPLETE -------");
	}

	void VulkanSurface::CreateSurface(VkInstance instance, SDL_Window* nativeWindow)
	{
		IG_CORE_TRACE("Creating Surface");

		if (!SDL_Vulkan_CreateSurface(nativeWindow, instance, nullptr, &m_Surface))
		{
			IG_CORE_CRITICAL("Failed SDL_Vulkan_CreateSurface: {}", SDL_GetError());

			m_Surface = VK_NULL_HANDLE;
			m_Instance = VK_NULL_HANDLE;

			return;
		}

		IG_CORE_TRACE("Surface Created");
	}

	void VulkanSurface::DestroySurface()
	{
		IG_CORE_TRACE("Destroying Surface");

		if (m_Surface == VK_NULL_HANDLE || m_Instance == VK_NULL_HANDLE)
		{
			return;
		}

		SDL_Vulkan_DestroySurface(m_Instance, m_Surface, nullptr);

		m_Surface = VK_NULL_HANDLE;
		m_Instance = VK_NULL_HANDLE;

		IG_CORE_TRACE("Surface Destroyed");
	}
}