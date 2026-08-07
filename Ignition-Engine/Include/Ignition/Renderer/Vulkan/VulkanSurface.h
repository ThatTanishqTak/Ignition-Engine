#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;

namespace Ignition
{
	class VulkanSurface
	{
	public:
		VulkanSurface();
		~VulkanSurface();

		VulkanSurface(const VulkanSurface&) = delete;
		VulkanSurface& operator=(const VulkanSurface&) = delete;

		void Initialize(VkInstance instance, SDL_Window* nativeWindow);
		void Shutdown();

		VkSurfaceKHR GetSurface() const { return m_Surface; }
		bool IsValid() const { return m_Surface != VK_NULL_HANDLE; }

	private:
		void CreateSurface(VkInstance instance, SDL_Window* nativeWindow);
		void DestroySurface();

	private:
		VkInstance m_Instance = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;
	};
}