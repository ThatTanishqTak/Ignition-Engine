#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

#include "Ignition/Renderer/Vulkan/VulkanInstance.h"
#include "Ignition/Renderer/Vulkan/VulkanSurface.h"
#include "Ignition/Renderer/Vulkan/VulkanDevice.h"
#include "Ignition/Renderer/Vulkan/VulkanSwapchain.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"
#include "Ignition/Renderer/Vulkan/VulkanAllocator.h"
#include "Ignition/Renderer/Vulkan/VulkanPipeline.h"
#include "Ignition/Renderer/Vulkan/VulkanMesh.h"
#include "Ignition/Renderer/Vulkan/VulkanImGui.h"
#include "Ignition/Renderer/Vulkan/VulkanLineRenderer.h"
#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Ignition/Renderer/Vulkan/VulkanFluidSolver2D.h"
#include "Ignition/Renderer/Vulkan/VulkanFluidSolver3D.h"
#include "Ignition/Renderer/Vulkan/VulkanGPUTimer.h"
#include "Ignition/Renderer/DebugDrawBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanTexture.h"
#include "Ignition/Renderer/Vulkan/VulkanImage.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Core/ProfilerInternal.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Renderer/Vulkan/VulkanShaderTypes.h"

#include <SDL3/SDL_video.h>
#include <SDL3/SDL_filesystem.h>

#include <array>
#include <cstdlib>
#include <cstring>
#include <string>

namespace Ignition
{
	namespace
	{
		constexpr const char* ShaderDirectory = "Shaders/";
		constexpr VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;
	}

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

		const VkExtent2D swapchainExtent = m_VulkanSwapchain->GetExtent();

		m_DepthImage = std::make_unique<VulkanImage>();
		m_DepthImage->Initialize(m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), swapchainExtent.width, swapchainExtent.height, DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

		if (!m_DepthImage->IsValid())
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no depth image");

			return;
		}

		m_VulkanFrameContext = std::make_unique<VulkanFrameContext>();
		m_VulkanFrameContext->Initialize(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueueFamily());

		if (!m_VulkanFrameContext->IsValid())
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no frame context");

			m_VulkanFrameContext->Shutdown();
			m_VulkanFrameContext.reset();

			return;
		}

		m_VulkanDescriptorAllocator = std::make_unique<VulkanDescriptorAllocator>();
		m_VulkanDescriptorAllocator->Initialize(m_VulkanDevice->GetDevice());

		if (!m_VulkanDescriptorAllocator->IsValid())
		{
			IG_CORE_CRITICAL("Vulkan renderer initialization aborted: no descriptor allocator");

			return;
		}

		const char* basePath = SDL_GetBasePath();
		const std::string shaderPath = std::string(basePath ? basePath : "") + ShaderDirectory + "Mesh.spv";

		m_VulkanPipeline = std::make_unique<VulkanPipeline>();
		m_VulkanPipeline->Initialize(m_VulkanDevice->GetDevice(), m_VulkanSwapchain->GetImageFormat(), DepthFormat, shaderPath, m_VulkanDescriptorAllocator->GetTextureSetLayout());

		if (!m_VulkanPipeline->IsValid())
		{
			IG_CORE_ERROR("Vulkan renderer: demo pipeline unavailable, the quad will not draw");
		}

		const std::string linePath = std::string(basePath ? basePath : "") + ShaderDirectory + "DebugLine.spv";

		m_VulkanLineRenderer = std::make_unique<VulkanLineRenderer>();
		m_VulkanLineRenderer->Initialize(m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), m_VulkanSwapchain->GetImageFormat(), DepthFormat, linePath);

		if (!m_VulkanLineRenderer->IsValid())
		{
			IG_CORE_ERROR("Vulkan renderer: debug line pipeline unavailable, DebugDraw output will be dropped");
		}

		m_VulkanGPUTimer = std::make_unique<VulkanGPUTimer>();
		m_VulkanGPUTimer->Initialize(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetTimestampPeriod());

		// Calibration records and submits on the given buffer, so this has to happen while nothing is in flight
		IG_PROFILE_GPU_CREATE(m_GPUProfiler, m_VulkanDevice->GetPhysicalDevice(), m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueue(), m_VulkanFrameContext->GetCommandBuffer(0));
		IG_PROFILE_GPU_NAME(m_GPUProfiler, "Graphics");

		constexpr uint8_t whitePixel[4] = { 255, 255, 255, 255 };

		m_WhiteTexture = std::make_unique<VulkanTexture>();
		m_WhiteTexture->Initialize(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueue(), m_VulkanDevice->GetGraphicsQueueFamily(), m_VulkanAllocator->GetAllocator(), *m_VulkanDescriptorAllocator, whitePixel, 1, 1);

		if (!m_WhiteTexture->IsValid())
		{
			IG_CORE_ERROR("Vulkan renderer: white fallback texture unavailable, untextured submits will not draw");

			m_WhiteTexture->Shutdown();
			m_WhiteTexture.reset();
		}

		m_VulkanImGui = std::make_unique<VulkanImGui>();
		m_VulkanImGui->Initialize(window, m_VulkanInstance->GetInstance(), m_VulkanDevice->GetPhysicalDevice(), m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueueFamily(), m_VulkanDevice->GetGraphicsQueue(), m_VulkanSwapchain->GetImageCount(), m_VulkanSwapchain->GetImageFormat(), DepthFormat);

		if (!m_VulkanImGui->IsValid())
		{
			IG_CORE_ERROR("Vulkan renderer: ImGui unavailable, debug UI disabled");
		}

		IG_CORE_INFO("------- VULKAN RENDERER INITIALIZED -------");
	}

	void VulkanRenderer::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN RENDERER -------");

		if (m_VulkanDevice && m_VulkanDevice->GetDevice() != VK_NULL_HANDLE)
		{
			VK_CHECK(vkDeviceWaitIdle(m_VulkanDevice->GetDevice()));
		}

		if (m_SelfReference)
		{
			*m_SelfReference = nullptr;
			m_SelfReference.reset();
		}

		m_ComputePasses.clear();

		DestroySceneRenderTarget();

		FlushRetirementQueue();

		if (m_LinearSampler != VK_NULL_HANDLE && m_VulkanDevice && m_VulkanDevice->GetDevice() != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_VulkanDevice->GetDevice(), m_LinearSampler, nullptr);
			m_LinearSampler = VK_NULL_HANDLE;
		}

		if (m_VulkanImGui)
		{
			m_VulkanImGui->Shutdown();
			m_VulkanImGui.reset();
		}

		IG_PROFILE_GPU_DESTROY(m_GPUProfiler);

		if (m_VulkanGPUTimer)
		{
			m_VulkanGPUTimer->Shutdown();
			m_VulkanGPUTimer.reset();
		}

		if (m_VulkanLineRenderer)
		{
			m_VulkanLineRenderer->Shutdown();
			m_VulkanLineRenderer.reset();
		}

		if (m_VulkanPipeline)
		{
			m_VulkanPipeline->Shutdown();
			m_VulkanPipeline.reset();
		}

		if (m_WhiteTexture)
		{
			m_WhiteTexture->Shutdown();
			m_WhiteTexture.reset();
		}

		if (m_VulkanDescriptorAllocator)
		{
			m_VulkanDescriptorAllocator->Shutdown();
			m_VulkanDescriptorAllocator.reset();
		}

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

		if (m_DepthImage)
		{
			m_DepthImage->Shutdown();
			m_DepthImage.reset();
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

	void VulkanRenderer::SetClearColor(float r, float g, float b, float a)
	{
		m_ClearColor[0] = r;
		m_ClearColor[1] = g;
		m_ClearColor[2] = b;
		m_ClearColor[3] = a;
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

		if (m_DepthImage && m_VulkanSwapchain->IsValid())
		{
			const VkExtent2D extent = m_VulkanSwapchain->GetExtent();

			if (extent.width != m_DepthImage->GetExtent().width || extent.height != m_DepthImage->GetExtent().height)
			{
				m_DepthImage->Shutdown();
				m_DepthImage->Initialize(m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), extent.width, extent.height, DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

				if (!m_DepthImage->IsValid())
				{
					IG_CORE_ERROR("Vulkan renderer: depth image recreation failed, rendering paused until the next resize");
				}
			}
		}
	}

	void VulkanRenderer::BeginFrame()
	{
		IG_PROFILE_ZONE();

		m_FrameStarted = false;

		if (!m_VulkanSwapchain || !m_VulkanFrameContext || !m_VulkanDevice || !m_DepthImage || !m_DepthImage->IsValid())
		{
			return;
		}

		if (m_SceneTargetResizeRequested)
		{
			CreateSceneRenderTarget(m_PendingSceneTargetWidth, m_PendingSceneTargetHeight);
			m_SceneTargetResizeRequested = false;
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

		VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));

		ProcessRetirementQueue();

		const VkSemaphore imageAvailable = m_VulkanFrameContext->GetImageAvailableSemaphore(m_FrameIndex);
		const VkResult acquireResult = vkAcquireNextImageKHR(device, m_VulkanSwapchain->GetSwapchain(), UINT64_MAX, imageAvailable, VK_NULL_HANDLE, &m_ImageIndex);

		if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
		{
			RecreateSwapchain();

			return;
		}

		if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
		{
			VK_CHECK(acquireResult);

			return;
		}

		const VkCommandBuffer commandBuffer = m_VulkanFrameContext->GetCommandBuffer(m_FrameIndex);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		if (!VK_CHECK(vkResetCommandBuffer(commandBuffer, 0)) || !VK_CHECK(vkBeginCommandBuffer(commandBuffer, &beginInfo)))
		{
			VkSemaphoreSubmitInfo drainWaitSubmitInfo{};
			drainWaitSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
			drainWaitSubmitInfo.semaphore = imageAvailable;
			drainWaitSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;

			VkSubmitInfo2 drainSubmitInfo{};
			drainSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
			drainSubmitInfo.waitSemaphoreInfoCount = 1;
			drainSubmitInfo.pWaitSemaphoreInfos = &drainWaitSubmitInfo;

			if (!VK_CHECK(vkQueueSubmit2(m_VulkanDevice->GetGraphicsQueue(), 1, &drainSubmitInfo, VK_NULL_HANDLE)))
			{
				IG_CORE_CRITICAL("Could not recover the image available semaphore, disabling the renderer");

				m_VulkanFrameContext->Shutdown();
				m_VulkanFrameContext.reset();

				return;
			}

			RecreateSwapchain();

			return;
		}

		if (m_VulkanGPUTimer)
		{
			m_VulkanGPUTimer->BeginFrame(commandBuffer, m_FrameIndex);
			m_FramePassTimer = m_VulkanGPUTimer->BeginPass(commandBuffer, "Frame");
		}

		// Compute cannot be recorded inside a dynamic rendering block, so every registered pass runs here
		{
			IG_PROFILE_ZONE_NAMED("Fluid Record");
			IG_PROFILE_GPU_ZONE(m_GPUProfiler, commandBuffer, "Fluid");

			for (VulkanComputePass* computePass : m_ComputePasses)
			{
				computePass->RecordCompute(commandBuffer, m_FrameIndex, m_VulkanGPUTimer.get());
			}
		}

		Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_VulkanSwapchain->GetImage(m_ImageIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

		Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_DepthImage->GetImage(), VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

		m_ScenePassActive = m_SceneColorImage && m_SceneColorImage->IsValid() && m_SceneDepthImage && m_SceneDepthImage->IsValid();

		if (m_ScenePassActive)
		{
			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_SceneColorImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_SceneDepthImage->GetImage(), VK_IMAGE_ASPECT_DEPTH_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
		}

		const VkExtent2D extent = m_ScenePassActive ? m_SceneColorImage->GetExtent() : m_VulkanSwapchain->GetExtent();

		VkRenderingAttachmentInfo renderingColorAttachmentInfo{};
		renderingColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		renderingColorAttachmentInfo.imageView = m_ScenePassActive ? m_SceneColorImage->GetImageView() : m_VulkanSwapchain->GetImageView(m_ImageIndex);
		renderingColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		renderingColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderingColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		renderingColorAttachmentInfo.clearValue.color = { { m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3] } };

		VkRenderingAttachmentInfo renderingDepthAttachmentInfo{};
		renderingDepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		renderingDepthAttachmentInfo.imageView = m_ScenePassActive ? m_SceneDepthImage->GetImageView() : m_DepthImage->GetImageView();
		renderingDepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
		renderingDepthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		renderingDepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		renderingDepthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = extent;
		renderingInfo.layerCount = 1;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &renderingColorAttachmentInfo;
		renderingInfo.pDepthAttachment = &renderingDepthAttachmentInfo;

		if (m_VulkanGPUTimer)
		{
			m_ScenePassTimer = m_VulkanGPUTimer->BeginPass(commandBuffer, "Scene");
		}

		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = static_cast<float>(extent.height);
		viewport.width = static_cast<float>(extent.width);
		viewport.height = -static_cast<float>(extent.height);
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
		IG_PROFILE_ZONE();

		if (!m_FrameStarted)
		{
			// Nothing recorded this frame; drop the queued lines so they cannot leak into the next one
			DebugDrawBuffer::Get().Clear();

			return;
		}

		const VkCommandBuffer commandBuffer = m_VulkanFrameContext->GetCommandBuffer(m_FrameIndex);

		// The scene (or main) pass is still open here, which is exactly where debug lines belong
		if (m_VulkanLineRenderer && m_VulkanLineRenderer->IsValid())
		{
			const DebugDrawBuffer& debugDrawBuffer = DebugDrawBuffer::Get();

			m_VulkanLineRenderer->Draw(commandBuffer, m_FrameIndex, m_SceneViewProjection, debugDrawBuffer.GetDepthTestedVertices(), debugDrawBuffer.GetOverlayVertices());
		}

		DebugDrawBuffer::Get().Clear();

		if (m_VulkanGPUTimer)
		{
			m_VulkanGPUTimer->EndPass(commandBuffer, m_ScenePassTimer);
		}

		if (m_ScenePassActive)
		{
			vkCmdEndRendering(commandBuffer);

			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_SceneColorImage->GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);

			const VkExtent2D swapchainExtent = m_VulkanSwapchain->GetExtent();

			VkRenderingAttachmentInfo compositeColorAttachmentInfo{};
			compositeColorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			compositeColorAttachmentInfo.imageView = m_VulkanSwapchain->GetImageView(m_ImageIndex);
			compositeColorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			compositeColorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			compositeColorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			compositeColorAttachmentInfo.clearValue.color = { { m_ClearColor[0], m_ClearColor[1], m_ClearColor[2], m_ClearColor[3] } };

			VkRenderingAttachmentInfo compositeDepthAttachmentInfo{};
			compositeDepthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			compositeDepthAttachmentInfo.imageView = m_DepthImage->GetImageView();
			compositeDepthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
			compositeDepthAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			compositeDepthAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			compositeDepthAttachmentInfo.clearValue.depthStencil = { 1.0f, 0 };

			VkRenderingInfo compositeRenderingInfo{};
			compositeRenderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
			compositeRenderingInfo.renderArea.offset = { 0, 0 };
			compositeRenderingInfo.renderArea.extent = swapchainExtent;
			compositeRenderingInfo.layerCount = 1;
			compositeRenderingInfo.colorAttachmentCount = 1;
			compositeRenderingInfo.pColorAttachments = &compositeColorAttachmentInfo;
			compositeRenderingInfo.pDepthAttachment = &compositeDepthAttachmentInfo;

			vkCmdBeginRendering(commandBuffer, &compositeRenderingInfo);
		}

		if (m_ImGuiFrameActive)
		{
			const uint32_t userInterfacePass = m_VulkanGPUTimer ? m_VulkanGPUTimer->BeginPass(commandBuffer, "UI") : UINT32_MAX;

			IG_PROFILE_GPU_ZONE(m_GPUProfiler, commandBuffer, "UI");

			m_VulkanImGui->Render(commandBuffer);

			if (m_VulkanGPUTimer)
			{
				m_VulkanGPUTimer->EndPass(commandBuffer, userInterfacePass);
			}

			m_ImGuiFrameActive = false;
		}

		vkCmdEndRendering(commandBuffer);

		m_ScenePassActive = false;

		// Must sit outside any rendering block, once per frame
		IG_PROFILE_GPU_COLLECT(m_GPUProfiler, commandBuffer);

		Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_VulkanSwapchain->GetImage(m_ImageIndex), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, 0);

		if (m_VulkanGPUTimer)
		{
			m_VulkanGPUTimer->EndPass(commandBuffer, m_FramePassTimer);
		}

		if (!VK_CHECK(vkEndCommandBuffer(commandBuffer)))
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

		if (!VK_CHECK(vkResetFences(device, 1, &fence)))
		{
			m_FrameStarted = false;

			return;
		}

		if (!VK_CHECK(vkQueueSubmit2(m_VulkanDevice->GetGraphicsQueue(), 1, &submitInfo, fence)))
		{
			IG_CORE_CRITICAL("Queue submission failed, the device may be lost, disabling the renderer");

			m_VulkanFrameContext->Shutdown();
			m_VulkanFrameContext.reset();

			m_FrameStarted = false;

			return;
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

		VkResult presentResult = VK_SUCCESS;
		{
			// Where the frame actually waits when the GPU is the bottleneck - worth its own zone
			IG_PROFILE_ZONE_NAMED("Present");

			presentResult = vkQueuePresentKHR(m_VulkanDevice->GetPresentQueue(), &presentInfo);
		}

		if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || m_ResizeRequested)
		{
			RecreateSwapchain();
		}
		else if (presentResult != VK_SUCCESS)
		{
			VK_CHECK(presentResult);
		}

		m_FrameStarted = false;
		m_FrameIndex = (m_FrameIndex + 1) % VulkanFrameContext::MaximumFramesInFlight;
		++m_FrameNumber;
	}

	void VulkanRenderer::ProcessImGuiEvent(const void* sdlEvent)
	{
		if (m_VulkanImGui)
		{
			m_VulkanImGui->ProcessEvent(sdlEvent);
		}
	}

	void VulkanRenderer::BeginImGuiFrame()
	{
		if (!m_FrameStarted || !m_VulkanImGui || !m_VulkanImGui->IsValid())
		{
			return;
		}

		m_VulkanImGui->BeginFrame();
		m_ImGuiFrameActive = true;
	}

	bool VulkanRenderer::WantCaptureMouse() const
	{
		return m_VulkanImGui && m_VulkanImGui->WantCaptureMouse();
	}

	bool VulkanRenderer::WantCaptureKeyboard() const
	{
		return m_VulkanImGui && m_VulkanImGui->WantCaptureKeyboard();
	}

	void VulkanRenderer::WaitIdle()
	{
		if (m_VulkanDevice && m_VulkanDevice->GetDevice() != VK_NULL_HANDLE)
		{
			VK_CHECK(vkDeviceWaitIdle(m_VulkanDevice->GetDevice()));
		}
	}

	std::unique_ptr<VulkanMesh> VulkanRenderer::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (!IsValid() || !m_VulkanAllocator)
		{
			return nullptr;
		}

		auto mesh = std::make_unique<VulkanMesh>();
		mesh->Initialize(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueue(), m_VulkanDevice->GetGraphicsQueueFamily(), m_VulkanAllocator->GetAllocator(), vertices, indices);

		if (!mesh->IsValid())
		{
			mesh->Shutdown();

			return nullptr;
		}

		return mesh;
	}

	void VulkanRenderer::BeginScene(const glm::mat4& viewProjection)
	{
		m_SceneActive = false;

		if (!m_FrameStarted || !m_VulkanPipeline || !m_VulkanPipeline->IsValid())
		{
			return;
		}

		m_SceneViewProjection = viewProjection;
		m_BoundPipelineVariant = -1;

		m_SceneActive = true;
	}

	std::unique_ptr<VulkanTexture> VulkanRenderer::CreateTexture(const std::string& filepath)
	{
		if (!IsValid() || !m_VulkanAllocator || !m_VulkanDescriptorAllocator)
		{
			return nullptr;
		}

		const char* basePath = SDL_GetBasePath();
		const std::string fullPath = std::string(basePath ? basePath : "") + filepath;

		auto texture = std::make_unique<VulkanTexture>();
		texture->InitializeFromFile(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueue(), m_VulkanDevice->GetGraphicsQueueFamily(), m_VulkanAllocator->GetAllocator(), *m_VulkanDescriptorAllocator, fullPath);

		if (!texture->IsValid())
		{
			texture->Shutdown();

			return nullptr;
		}

		return texture;
	}

	std::unique_ptr<VulkanTexture> VulkanRenderer::CreateTextureFromMemory(const void* data, size_t size)
	{
		if (!IsValid() || !m_VulkanAllocator || !m_VulkanDescriptorAllocator)
		{
			return nullptr;
		}

		auto texture = std::make_unique<VulkanTexture>();
		texture->InitializeFromMemory(m_VulkanDevice->GetDevice(), m_VulkanDevice->GetGraphicsQueue(), m_VulkanDevice->GetGraphicsQueueFamily(), m_VulkanAllocator->GetAllocator(), *m_VulkanDescriptorAllocator, data, size);

		if (!texture->IsValid())
		{
			texture->Shutdown();

			return nullptr;
		}

		return texture;
	}

	void VulkanRenderer::Submit(const VulkanMesh& mesh, const glm::mat4& transform)
	{
		Submit(mesh, nullptr, glm::vec4(1.0f), false, transform);
	}

	void VulkanRenderer::Submit(const VulkanMesh& mesh, const VulkanTexture* texture, const glm::vec4& tint, bool twoSided, const glm::mat4& transform)
	{
		if (!m_SceneActive || !mesh.IsValid())
		{
			return;
		}

		const VulkanTexture* boundTexture = (texture && texture->IsValid()) ? texture : m_WhiteTexture.get();

		if (!boundTexture || !boundTexture->IsValid())
		{
			return;
		}

		const VkCommandBuffer commandBuffer = m_VulkanFrameContext->GetCommandBuffer(m_FrameIndex);

		const int pipelineVariant = twoSided ? 1 : 0;

		if (pipelineVariant != m_BoundPipelineVariant)
		{
			m_VulkanPipeline->Bind(commandBuffer, twoSided);
			m_BoundPipelineVariant = pipelineVariant;
		}

		const VkDescriptorSet descriptorSet = boundTexture->GetDescriptorSet();

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_VulkanPipeline->GetPipelineLayout(), 0, 1, &descriptorSet, 0, nullptr);

		PushConstantData pushConstantData{};
		pushConstantData.MVP = m_SceneViewProjection * transform;
		pushConstantData.Tint = tint;

		vkCmdPushConstants(commandBuffer, m_VulkanPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantData), &pushConstantData);

		mesh.Bind(commandBuffer);

		vkCmdDrawIndexed(commandBuffer, mesh.GetIndexCount(), 1, 0, 0, 0);
	}

	void VulkanRenderer::EndScene()
	{
		m_SceneActive = false;
	}

	void VulkanRenderer::SetSceneRenderTargetSize(uint32_t width, uint32_t height)
	{
		if (width == GetSceneRenderTargetWidth() && height == GetSceneRenderTargetHeight())
		{
			return;
		}

		m_PendingSceneTargetWidth = width;
		m_PendingSceneTargetHeight = height;
		m_SceneTargetResizeRequested = true;
	}

	uint64_t VulkanRenderer::GetSceneRenderTargetTextureID() const
	{
		return reinterpret_cast<uint64_t>(m_SceneTextureDescriptor);
	}

	uint32_t VulkanRenderer::GetSceneRenderTargetWidth() const
	{
		return m_SceneColorImage && m_SceneColorImage->IsValid() ? m_SceneColorImage->GetExtent().width : 0;
	}

	uint32_t VulkanRenderer::GetSceneRenderTargetHeight() const
	{
		return m_SceneColorImage && m_SceneColorImage->IsValid() ? m_SceneColorImage->GetExtent().height : 0;
	}

	namespace
	{
		struct RetiredImGuiTexture
		{
			VulkanImGui* ImGui = nullptr;
			VkDescriptorSet DescriptorSet = VK_NULL_HANDLE;

			void Shutdown()
			{
				if (ImGui && DescriptorSet != VK_NULL_HANDLE)
				{
					ImGui->RemoveTexture(DescriptorSet);
				}
			}
		};
	}

	VkSampler VulkanRenderer::EnsureLinearSampler()
	{
		if (m_LinearSampler != VK_NULL_HANDLE || !m_VulkanDevice)
		{
			return m_LinearSampler;
		}

		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		if (!VK_CHECK(vkCreateSampler(m_VulkanDevice->GetDevice(), &samplerCreateInfo, nullptr, &m_LinearSampler)))
		{
			m_LinearSampler = VK_NULL_HANDLE;
		}

		return m_LinearSampler;
	}

	VkDescriptorSet VulkanRenderer::AddImGuiTexture(VkImageView imageView)
	{
		const VkSampler sampler = EnsureLinearSampler();

		if (sampler == VK_NULL_HANDLE || imageView == VK_NULL_HANDLE || !m_VulkanImGui || !m_VulkanImGui->IsValid())
		{
			return VK_NULL_HANDLE;
		}

		return m_VulkanImGui->AddTexture(sampler, imageView);
	}

	void VulkanRenderer::RemoveImGuiTexture(VkDescriptorSet descriptorSet)
	{
		if (m_VulkanImGui)
		{
			m_VulkanImGui->RemoveTexture(descriptorSet);
		}
	}

	const std::vector<PassTiming>& VulkanRenderer::GetPassTimings() const
	{
		static const std::vector<PassTiming> empty;

		return m_VulkanGPUTimer ? m_VulkanGPUTimer->GetResults() : empty;
	}

	std::unique_ptr<VulkanFluidSolver2D> VulkanRenderer::CreateFluidSolver2D(const FluidSolver2DSettings& settings)
	{
		if (!IsValid() || !m_VulkanAllocator || !m_VulkanDescriptorAllocator)
		{
			return nullptr;
		}

		const char* basePath = SDL_GetBasePath();
		const std::string shaderPath = std::string(basePath ? basePath : "") + ShaderDirectory + "Fluid2D.spv";

		auto solver = std::make_unique<VulkanFluidSolver2D>();
		solver->Initialize(*this, m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), *m_VulkanDescriptorAllocator, shaderPath, settings);

		if (!solver->IsValid())
		{
			solver->Shutdown();

			return nullptr;
		}

		m_ComputePasses.push_back(solver.get());

		return solver;
	}

	std::unique_ptr<VulkanFluidSolver3D> VulkanRenderer::CreateFluidSolver3D(const FluidSolver3DSettings& settings)
	{
		if (!IsValid() || !m_VulkanAllocator || !m_VulkanDescriptorAllocator)
		{
			return nullptr;
		}

		const char* basePath = SDL_GetBasePath();
		const std::string shaderPath = std::string(basePath ? basePath : "") + ShaderDirectory + "Fluid3D.spv";

		auto solver = std::make_unique<VulkanFluidSolver3D>();
		solver->Initialize(m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), *m_VulkanDescriptorAllocator, shaderPath, settings);

		if (!solver->IsValid())
		{
			solver->Shutdown();

			return nullptr;
		}

		m_ComputePasses.push_back(solver.get());

		return solver;
	}

	void VulkanRenderer::Retire(std::unique_ptr<VulkanFluidSolver2D> solver)
	{
		if (!solver)
		{
			return;
		}

		std::erase(m_ComputePasses, static_cast<VulkanComputePass*>(solver.get()));

		RetireResource(std::move(solver));
	}

	void VulkanRenderer::Retire(std::unique_ptr<VulkanFluidSolver3D> solver)
	{
		if (!solver)
		{
			return;
		}

		std::erase(m_ComputePasses, static_cast<VulkanComputePass*>(solver.get()));

		RetireResource(std::move(solver));
	}

	void VulkanRenderer::CreateSceneRenderTarget(uint32_t width, uint32_t height)
	{
		DestroySceneRenderTarget();

		if (width == 0 || height == 0 || !m_VulkanDevice || !m_VulkanAllocator || !m_VulkanSwapchain)
		{
			return;
		}

		m_SceneColorImage = std::make_unique<VulkanImage>();
		m_SceneColorImage->Initialize(m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), width, height, m_VulkanSwapchain->GetImageFormat(), VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		m_SceneDepthImage = std::make_unique<VulkanImage>();
		m_SceneDepthImage->Initialize(m_VulkanDevice->GetDevice(), m_VulkanAllocator->GetAllocator(), width, height, DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_IMAGE_ASPECT_DEPTH_BIT);

		if (!m_SceneColorImage->IsValid() || !m_SceneDepthImage->IsValid())
		{
			IG_CORE_ERROR("Vulkan renderer: scene render target creation failed ({}x{})", width, height);
			DestroySceneRenderTarget();

			return;
		}

		m_SceneTextureDescriptor = AddImGuiTexture(m_SceneColorImage->GetImageView());

		if (m_SceneTextureDescriptor == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("Vulkan renderer: scene render target could not be published to ImGui");
		}
	}

	void VulkanRenderer::DestroySceneRenderTarget()
	{
		if (m_SceneTextureDescriptor != VK_NULL_HANDLE)
		{
			RetireResource(std::make_unique<RetiredImGuiTexture>(RetiredImGuiTexture{ m_VulkanImGui.get(), m_SceneTextureDescriptor }));

			m_SceneTextureDescriptor = VK_NULL_HANDLE;
		}

		RetireResource(std::move(m_SceneColorImage));
		RetireResource(std::move(m_SceneDepthImage));
	}

	template <typename TResource>
	void VulkanRenderer::RetireResource(std::unique_ptr<TResource> resource)
	{
		if (!resource)
		{
			return;
		}

		if (!m_VulkanDevice || m_VulkanDevice->GetDevice() == VK_NULL_HANDLE)
		{
			resource->Shutdown();

			return;
		}

		std::shared_ptr<void> owned(resource.release(), [](TResource* pointer)
		{
			pointer->Shutdown();
			delete pointer;
		});

		m_RetirementQueue.push_back({ std::move(owned), m_FrameNumber });
	}

	void VulkanRenderer::Retire(std::unique_ptr<VulkanMesh> mesh)
	{
		RetireResource(std::move(mesh));
	}

	void VulkanRenderer::Retire(std::unique_ptr<VulkanTexture> texture)
	{
		RetireResource(std::move(texture));
	}

	void VulkanRenderer::Retire(std::unique_ptr<VulkanImage> image)
	{
		RetireResource(std::move(image));
	}

	void VulkanRenderer::ProcessRetirementQueue()
	{
		while (!m_RetirementQueue.empty() && m_RetirementQueue.front().FrameNumber + VulkanFrameContext::MaximumFramesInFlight <= m_FrameNumber)
		{
			m_RetirementQueue.pop_front();
		}
	}

	void VulkanRenderer::FlushRetirementQueue()
	{
		m_RetirementQueue.clear();
	}
}