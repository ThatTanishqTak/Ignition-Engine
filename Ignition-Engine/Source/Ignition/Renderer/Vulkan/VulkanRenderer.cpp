#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

#include "Ignition/Renderer/Vulkan/VulkanInstance.h"
#include "Ignition/Renderer/Vulkan/VulkanSurface.h"
#include "Ignition/Renderer/Vulkan/VulkanDevice.h"
#include "Ignition/Renderer/Vulkan/VulkanSwapchain.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"
#include "Ignition/Renderer/Vulkan/VulkanAllocator.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include <SDL3/SDL_video.h>

#include <cstdlib>

namespace Ignition
{
	VulkanRenderer::VulkanRenderer() = default;
	VulkanRenderer::~VulkanRenderer() = default;

	void VulkanRenderer::Initialize(SDL_Window* window)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN RENDERER -------");

		m_Window = window;

		m_VulkanInstance = std::make_unique<VulkanInstance>();
		m_VulkanInstance->Initialize();

		if (m_VulkanInstance->GetInstance() == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no instance");

			return;
		}

		m_VulkanSurface = std::make_unique<VulkanSurface>();
		m_VulkanSurface->Initialize(m_VulkanInstance->GetInstance(), window);

		if (!m_VulkanSurface->IsValid())
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no surface");

			return;
		}

		m_VulkanDevice = std::make_unique<VulkanDevice>();
		m_VulkanDevice->Initialize(m_VulkanInstance->GetInstance(), m_VulkanSurface->GetSurface());

		if (m_VulkanDevice->GetDevice() == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no device");

			return;
		}

		m_VulkanAllocator = std::make_unique<VulkanAllocator>();
		m_VulkanAllocator->Initialize(m_VulkanInstance->GetInstance(), m_VulkanDevice->GetPhysicalDevice(), m_VulkanDevice->GetDevice(), m_VulkanInstance->GetAPIVersion());

		if (m_VulkanAllocator->GetAllocator() == VK_NULL_HANDLE)
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no allocator");

			return;
		}

		uint32_t width = 0;
		uint32_t height = 0;
		GetWindowPixelSize(width, height);

		m_VulkanSwapchain = std::make_unique<VulkanSwapchain>();
		m_VulkanSwapchain->Initialize(m_VulkanDevice->GetPhysicalDevice(), m_VulkanDevice->GetDevice(), m_VulkanSurface->GetSurface(), m_VulkanDevice->GetGraphicsQueueFamily(), m_VulkanDevice->GetPresentQueueFamily(), width, height);

		m_VulkanFrameContext = std::make_unique<VulkanFrameContext>();
		m_VulkanFrameContext->Initialize(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueueFamily());

		if (!m_VulkanFrameContext->IsValid())
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no frame context");

			m_VulkanFrameContext->Shutdown();
			m_VulkanFrameContext.reset();

			return;
		}

		IG_CORE_INFO("------- VULKAN RENDERER INITIALIZED -------");
	}

	void VulkanRenderer::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN RENDERER -------");

		if (m_VulkanFrameContext)
		{
			m_VulkanFrameContext->Shutdown();
			m_VulkanFrameContext.reset();
		}

		if (m_VulkanSwapchain)
		{
			m_VulkanSwapchain->Shutdown();
			m_VulkanSwapchain.reset();
		}

		if (m_VulkanAllocator)
		{
			m_VulkanAllocator->Shutdown();
			m_VulkanAllocator.reset();
		}

		if (m_VulkanDevice)
		{
			m_VulkanDevice->Shutdown();
			m_VulkanDevice.reset();
		}

		if (m_VulkanSurface)
		{
			m_VulkanSurface->Shutdown();
			m_VulkanSurface.reset();
		}

		if (m_VulkanInstance)
		{
			m_VulkanInstance->Shutdown();
			m_VulkanInstance.reset();
		}

		IG_CORE_INFO("------- VULKAN RENDERER SHUTDOWN COMPLETE -------");
	}

	void VulkanRenderer::SetClearColor(float r, float g, float b)
	{
		m_ClearColor[0] = r;
		m_ClearColor[1] = g;
		m_ClearColor[2] = b;
		m_ClearColor[3] = 1.0f;
	}

	void VulkanRenderer::OnResize()
	{
		m_ResizeRequested = true;
	}

	void VulkanRenderer::GetWindowPixelSize(uint32_t& outWidth, uint32_t& outHeight) const
	{
		int width = 0;
		int height = 0;

		if (m_Window)
		{
			SDL_GetWindowSizeInPixels(m_Window, &width, &height);
		}

		outWidth = static_cast<uint32_t>(width < 0 ? 0 : width);
		outHeight = static_cast<uint32_t>(height < 0 ? 0 : height);
	}

	void VulkanRenderer::RecreateSwapchain()
	{
		uint32_t width = 0;
		uint32_t height = 0;
		GetWindowPixelSize(width, height);

		m_VulkanSwapchain->Recreate(width, height);
		m_ResizeRequested = false;
	}

	void VulkanRenderer::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 sourceStage, VkAccessFlags2 sourceAccess, VkPipelineStageFlags2 destinationStage, VkAccessFlags2 destinationAccess)
	{
		VkImageMemoryBarrier2 imageMemoryBarrier{};
		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		imageMemoryBarrier.srcStageMask = sourceStage;
		imageMemoryBarrier.srcAccessMask = sourceAccess;
		imageMemoryBarrier.dstStageMask = destinationStage;
		imageMemoryBarrier.dstAccessMask = destinationAccess;
		imageMemoryBarrier.oldLayout = oldLayout;
		imageMemoryBarrier.newLayout = newLayout;
		imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
		imageMemoryBarrier.subresourceRange.levelCount = 1;
		imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
		imageMemoryBarrier.subresourceRange.layerCount = 1;

		VkDependencyInfo dependencyInfo{};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &imageMemoryBarrier;

		vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
	}

	void VulkanRenderer::BeginFrame()
	{
		m_FrameStarted = false;

		if (!m_VulkanSwapchain || !m_VulkanFrameContext || !m_VulkanDevice)
		{
			return;
		}

		if (m_ResizeRequested || !m_VulkanSwapchain->IsValid())
		{
			RecreateSwapchain();

			if (!m_VulkanSwapchain->IsValid())
			{
				return;
			}
		}

		const VkDevice device = m_VulkanDevice->GetDevice();
		const VkFence fence = m_VulkanFrameContext->GetInFlightFence(m_FrameIndex);

		Utilities::VulkanUtilities::VKCheck(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX), "Failed vkWaitForFences");

		const VkSemaphore imageAvailable = m_VulkanFrameContext->GetImageAvailableSemaphore(m_FrameIndex);
		const VkResult acquireResult = vkAcquireNextImageKHR(device, m_VulkanSwapchain->GetSwapchain(), UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &m_ImageIndex);

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();

			return;
		}

		if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		{
			Utilities::VulkanUtilities::VKCheck(acquireResult, "Failed vkAcquireNextImageKHR");

			return;
		}

		const VkCommandBuffer commandBuffer = m_VulkanFrameContext->GetCommandBuffer(m_FrameIndex);
		if (Utilities::VulkanUtilities::VKCheck(vkResetCommandBuffer(commandBuffer, 0), "Failed vkResetCommandBuffer"))
		{
			return;
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (Utilities::VulkanUtilities::VKCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed vkBeginCommandBuffer"))
		{
			return;
		}

		TransitionImageLayout(commandBuffer, m_VulkanSwapchain->GetImage(m_ImageIndex), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

		const VkExtent2D extent = m_VulkanSwapchain->GetExtent();

		VkRenderingAttachmentInfo renderingColorAttachmentInfo{};
		renderingColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		renderingColorAttachmentInfo.imageView = m_VulkanSwapchain->GetImageView(m_ImageIndex);
		renderingColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderingColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderingColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingColorAttachmentInfo.clearValue.color = { { m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3] } };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &renderingColorAttachmentInfo;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = extent;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		m_FrameStarted = true;
	}

	void VulkanRenderer::EndFrame()
	{
		if (!m_FrameStarted)
		{
			return;
		}

		const VkCommandBuffer commandBuffer = m_VulkanFrameContext->GetCommandBuffer(m_FrameIndex);

		vkCmdEndRendering(commandBuffer);

		TransitionImageLayout(commandBuffer, m_VulkanSwapchain->GetImage(m_ImageIndex), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

		if (Utilities::VulkanUtilities::VKCheck(vkEndCommandBuffer(commandBuffer), "Failed vkEndCommandBuffer"))
		{
			m_FrameStarted = false;

			return;
		}

		VkSemaphoreSubmitInfo waitSemaphoreSubmitInfo{};
		waitSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphoreSubmitInfo.semaphore = m_VulkanFrameContext->GetImageAvailableSemaphore(m_FrameIndex);
		waitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSemaphoreSubmitInfo signalSemaphoreSubmitInfo{};
		signalSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphoreSubmitInfo.semaphore = m_VulkanSwapchain->GetRenderFinishedSemaphore(m_ImageIndex);
		signalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;

		VkCommandBufferSubmitInfo commandBufferInfo{};
		commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		commandBufferInfo.commandBuffer = commandBuffer;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.waitSemaphoreInfoCount = 1;
		submitInfo.pWaitSemaphoreInfos = &waitSemaphoreSubmitInfo;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = &commandBufferInfo;
		submitInfo.signalSemaphoreInfoCount = 1;
		submitInfo.pSignalSemaphoreInfos = &signalSemaphoreSubmitInfo;

		const VkDevice device = m_VulkanDevice->GetDevice();
		const VkFence fence = m_VulkanFrameContext->GetInFlightFence(m_FrameIndex);
		Utilities::VulkanUtilities::VKCheck(vkResetFences(device, 1, &fence), "Failed vkResetFences");

		if (Utilities::VulkanUtilities::VKCheck(vkQueueSubmit2(m_VulkanDevice->GetGraphicsQueue(), 1, &submitInfo, fence), "Failed vkQueueSubmit2"))
		{
			IG_CORE_CRITICAL("Queue submission failed, the device may be lost");
		}

		const VkSemaphore renderFinished = m_VulkanSwapchain->GetRenderFinishedSemaphore(m_ImageIndex);
		const VkSwapchainKHR swapchain = m_VulkanSwapchain->GetSwapchain();

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = &renderFinished;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapchain;
		presentInfo.pImageIndices = &m_ImageIndex;

		const VkResult presentResult = vkQueuePresentKHR(m_VulkanDevice->GetPresentQueue(), &presentInfo);

		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_ResizeRequested)
		{
			RecreateSwapchain();
		}
		else if (presentResult != VK_SUCCESS)
		{
			Utilities::VulkanUtilities::VKCheck(presentResult, "Failed vkQueuePresentKHR");
		}

		m_FrameStarted = false;
		m_FrameIndex = (m_FrameIndex + 1) % VulkanFrameContext::MaximumFramesInFlight;
	}
}