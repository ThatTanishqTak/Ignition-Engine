#pragma once

#include <vulkan/vulkan.h>

#include <memory>
#include <cstdint>

struct SDL_Window;

namespace Ignition
{
	class VulkanInstance;
	class VulkanSurface;
	class VulkanDevice;
	class VulkanAllocator;
	class VulkanSwapchain;
	class VulkanFrameContext;

	class VulkanRenderer
	{
	public:
		VulkanRenderer();
		~VulkanRenderer();

		void Initialize(SDL_Window* window);
		void Shutdown();

		bool IsValid() const { return m_VulkanFrameContext != nullptr; }

		void SetClearColor(float r, float g, float b, float a = 1.0f);

		void BeginFrame();
		void EndFrame();

		void OnResize();

	private:
		void RecreateSwapchain();
		void GetWindowPixelSize(uint32_t& outWidth, uint32_t& outHeight) const;

		void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess);

	private:
		SDL_Window* m_Window = nullptr;

		std::unique_ptr<VulkanInstance> m_VulkanInstance;
		std::unique_ptr<VulkanSurface> m_VulkanSurface;
		std::unique_ptr<VulkanDevice> m_VulkanDevice;
		std::unique_ptr<VulkanAllocator> m_VulkanAllocator;
		std::unique_ptr<VulkanSwapchain> m_VulkanSwapchain;
		std::unique_ptr<VulkanFrameContext> m_VulkanFrameContext;

		uint32_t m_FrameIndex = 0;
		uint32_t m_ImageIndex = 0;
		bool m_ResizeRequested = false;
		bool m_FrameStarted = false;

		float m_ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	};
}