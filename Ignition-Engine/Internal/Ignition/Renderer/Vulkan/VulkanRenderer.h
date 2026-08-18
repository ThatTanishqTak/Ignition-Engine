#pragma once

#include <vulkan/vulkan.h>

#include "Ignition/Core/ProfilerVulkan.h"
#include "Ignition/Fluid/FluidSolver2D.h"
#include "Ignition/Fluid/FluidSolver3D.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePass.h"
#include "Ignition/Renderer/Vertex.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

struct SDL_Window;

namespace Ignition
{
	class VulkanInstance;
	class VulkanSurface;
	class VulkanDevice;
	class VulkanAllocator;
	class VulkanSwapchain;
	class VulkanFrameContext;
	class VulkanPipeline;
	class VulkanMesh;
	class VulkanImGui;
	class VulkanLineRenderer;
	class VulkanDescriptorAllocator;
	class VulkanTexture;
	class VulkanImage;
	class VulkanGPUTimer;
	class VulkanFluidSolver2D;
	class VulkanFluidSolver3D;

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

		void WaitIdle();

		void ProcessImGuiEvent(const void* sdlEvent);
		void BeginImGuiFrame();
		bool IsImGuiFrameActive() const { return m_ImGuiFrameActive; }

		bool WantCaptureMouse() const;
		bool WantCaptureKeyboard() const;

		std::unique_ptr<VulkanMesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		std::unique_ptr<VulkanTexture> CreateTexture(const std::string& filepath);
		std::unique_ptr<VulkanTexture> CreateTextureFromMemory(const void* data, size_t size);
		std::unique_ptr<VulkanFluidSolver2D> CreateFluidSolver2D(const FluidSolver2DSettings& settings);
		std::unique_ptr<VulkanFluidSolver3D> CreateFluidSolver3D(const FluidSolver3DSettings& settings);

		void Retire(std::unique_ptr<VulkanMesh> mesh);
		void Retire(std::unique_ptr<VulkanTexture> texture);
		void Retire(std::unique_ptr<VulkanImage> image);
		void Retire(std::unique_ptr<VulkanFluidSolver2D> solver);
		void Retire(std::unique_ptr<VulkanFluidSolver3D> solver);

		// Publishes a renderer-owned image view to ImGui with the shared linear sampler
		VkDescriptorSet AddImGuiTexture(VkImageView imageView);
		void RemoveImGuiTexture(VkDescriptorSet descriptorSet);

		const std::vector<PassTiming>& GetPassTimings() const;

		std::shared_ptr<VulkanRenderer*> GetSelfReference() const { return m_SelfReference; }

		void BeginScene(const glm::mat4& viewProjection);
		void Submit(const VulkanMesh& mesh, const glm::mat4& transform);
		void Submit(const VulkanMesh& mesh, const VulkanTexture* texture, const glm::vec4& tint, bool twoSided, const glm::mat4& transform);
		void EndScene();

		void SetSceneRenderTargetSize(uint32_t width, uint32_t height);
		uint64_t GetSceneRenderTargetTextureID() const;
		uint32_t GetSceneRenderTargetWidth() const;
		uint32_t GetSceneRenderTargetHeight() const;

		void OnResize();

	private:
		void RecreateSwapchain();
		VkSampler EnsureLinearSampler();
		void CreateSceneRenderTarget(uint32_t width, uint32_t height);
		void DestroySceneRenderTarget();
		void GetWindowPixelSize(uint32_t& outWidth, uint32_t& outHeight) const;
		void ProcessRetirementQueue();
		void FlushRetirementQueue();

		template <typename TResource>
		void RetireResource(std::unique_ptr<TResource> resource);

	private:
		struct RetiredResource
		{
			std::shared_ptr<void> Resource;
			uint64_t FrameNumber = 0;
		};

		SDL_Window* m_Window = nullptr;

		std::unique_ptr<VulkanInstance> m_VulkanInstance;
		std::unique_ptr<VulkanSurface> m_VulkanSurface;
		std::unique_ptr<VulkanDevice> m_VulkanDevice;
		std::unique_ptr<VulkanAllocator> m_VulkanAllocator;
		std::unique_ptr<VulkanSwapchain> m_VulkanSwapchain;
		std::unique_ptr<VulkanImage> m_DepthImage;
		std::unique_ptr<VulkanFrameContext> m_VulkanFrameContext;
		std::unique_ptr<VulkanDescriptorAllocator> m_VulkanDescriptorAllocator;
		std::unique_ptr<VulkanPipeline> m_VulkanPipeline;
		std::unique_ptr<VulkanLineRenderer> m_VulkanLineRenderer;
		std::unique_ptr<VulkanGPUTimer> m_VulkanGPUTimer;
		std::unique_ptr<VulkanTexture> m_WhiteTexture;
		std::unique_ptr<VulkanMesh> m_OverlayQuad; // unit quad for fluid slice planes and other in-scene overlays
		std::unique_ptr<VulkanImGui> m_VulkanImGui;

		// Compute work is recorded at the top of the frame, before any rendering begins - one slot, whatever registers into it
		std::vector<VulkanComputePass*> m_ComputePasses;

		uint32_t m_FramePassTimer = UINT32_MAX;
		uint32_t m_ScenePassTimer = UINT32_MAX;

		GPUProfilerContext m_GPUProfiler = nullptr;

		std::unique_ptr<VulkanImage> m_SceneColorImage;
		std::unique_ptr<VulkanImage> m_SceneDepthImage;
		VkSampler m_LinearSampler = VK_NULL_HANDLE;
		VkDescriptorSet m_SceneTextureDescriptor = VK_NULL_HANDLE;
		uint32_t m_PendingSceneTargetWidth = 0;
		uint32_t m_PendingSceneTargetHeight = 0;
		bool m_SceneTargetResizeRequested = false;
		bool m_ScenePassActive = false;

		glm::mat4 m_SceneViewProjection{ 1.0f };
		bool m_SceneActive = false;
		bool m_ImGuiFrameActive = false;
		int m_BoundPipelineVariant = -1;

		uint32_t m_FrameIndex = 0;
		uint32_t m_ImageIndex = 0;
		uint64_t m_FrameNumber = 0;
		bool m_ResizeRequested = false;
		bool m_FrameStarted = false;

		std::deque<RetiredResource> m_RetirementQueue;
		std::shared_ptr<VulkanRenderer*> m_SelfReference = std::make_shared<VulkanRenderer*>(this);

		float m_ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	};
}