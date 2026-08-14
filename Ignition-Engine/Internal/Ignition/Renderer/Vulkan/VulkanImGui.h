#pragma once

#include <vulkan/vulkan.h>

struct SDL_Window;

namespace Ignition
{
	class VulkanImGui
	{
	public:
		VulkanImGui();
		~VulkanImGui();

		VulkanImGui(const VulkanImGui&) = delete;
		VulkanImGui& operator=(const VulkanImGui&) = delete;

		void Initialize(SDL_Window* window, VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, uint32_t graphicsQueueFamily, VkQueue graphicsQueue, uint32_t swapchainImageCount, VkFormat colorFormat, VkFormat depthFormat);
		void Shutdown();

		bool IsValid() const { return m_Initialized; }

		void ProcessEvent(const void* sdlEvent);

		void BeginFrame();
		void Render(VkCommandBuffer commandBuffer);

		VkDescriptorSet AddTexture(VkSampler sampler, VkImageView imageView);
		void RemoveTexture(VkDescriptorSet descriptorSet);

		bool WantCaptureMouse() const;
		bool WantCaptureKeyboard() const;

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
		VkFormat m_ColorFormat = VK_FORMAT_UNDEFINED;

		bool m_Initialized = false;
	};
}