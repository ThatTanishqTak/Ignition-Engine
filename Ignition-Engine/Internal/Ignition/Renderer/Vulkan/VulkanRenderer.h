#pragma once

#include <vulkan/vulkan.h>

#include "Ignition/Core/ProfilerVulkan.h"
#include "Ignition/Fluid/FluidSolver3D.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Vulkan/VulkanComputePass.h"
#include "Ignition/Renderer/Vertex.h"
#include "Ignition/UI/UITypes.h"

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
	class VulkanLineRenderer;
	class VulkanDescriptorAllocator;
	class VulkanTexture;
	class VulkanImage;
	class VulkanGPUTimer;
	class VulkanFluidSolver3D;
	class VulkanUIRenderer;
	class VulkanUITextureTable;

	namespace UI
	{
		class DrawList;
	}

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

		std::unique_ptr<VulkanMesh> CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		std::unique_ptr<VulkanTexture> CreateTexture(const std::string& filepath);
		std::unique_ptr<VulkanTexture> CreateTextureFromMemory(const void* data, size_t size);
		std::unique_ptr<VulkanFluidSolver3D> CreateFluidSolver3D(const FluidSolver3DSettings& settings);

		void Retire(std::unique_ptr<VulkanMesh> mesh);
		void Retire(std::unique_ptr<VulkanTexture> texture);
		void Retire(std::unique_ptr<VulkanImage> image);
		void Retire(std::unique_ptr<VulkanFluidSolver3D> solver);

		const std::vector<PassTiming>& GetPassTimings() const;

		// A DrawList submitted between BeginFrame and EndFrame, recorded at its surface's insertion point. The pointer is borrowed for the rest of the frame: the caller owns the list and must not rebuild it before EndFrame
		void SubmitUI(const UI::DrawList& drawList, UI::UISurfaceTarget target);

		// Bindless slots for anything that wants to be drawn as an image by the UI. Zero is the permanent white
		uint32_t AcquireUITextureSlot(VkImageView imageView);
		void ReleaseUITextureSlot(uint32_t slot);

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
		std::unique_ptr<VulkanUITextureTable> m_UITextureTable;
		std::unique_ptr<VulkanUIRenderer> m_VulkanUIRenderer;

		std::array<const UI::DrawList*, static_cast<size_t>(UI::UISurfaceTarget::Count)> m_UIDrawLists{};

		// Compute work is recorded at the top of the frame, before any rendering begins - one slot, whatever registers into it
		std::vector<VulkanComputePass*> m_ComputePasses;

		uint32_t m_FramePassTimer = UINT32_MAX;
		uint32_t m_ScenePassTimer = UINT32_MAX;

		GPUProfilerContext m_GPUProfiler = nullptr;

		std::unique_ptr<VulkanImage> m_SceneColorImage;
		std::unique_ptr<VulkanImage> m_SceneDepthImage;
		uint32_t m_SceneColorSlot = 0;
		VkSampler m_LinearSampler = VK_NULL_HANDLE; // The UI texture table's shared sampler
		uint32_t m_PendingSceneTargetWidth = 0;
		uint32_t m_PendingSceneTargetHeight = 0;
		bool m_SceneTargetResizeRequested = false;
		bool m_ScenePassActive = false;

		glm::mat4 m_SceneViewProjection{ 1.0f };
		bool m_SceneActive = false;
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