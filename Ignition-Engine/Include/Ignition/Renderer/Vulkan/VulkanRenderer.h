#pragma once

#include <vulkan/vulkan.h>

#include "Ignition/Renderer/Vertex.h"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <memory>
#include <cstdint>
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
	class VulkanDescriptorAllocator;
	class VulkanTexture;
	class VulkanImage;

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

		void BeginScene(const glm::mat4& viewProjection);
		void Submit(const VulkanMesh& mesh, const glm::mat4& transform);
		void Submit(const VulkanMesh& mesh, const VulkanTexture* texture, const glm::vec4& tint, const glm::mat4& transform);
		void EndScene();

		void OnResize();

	private:
		void RecreateSwapchain();
		void GetWindowPixelSize(uint32_t& outWidth, uint32_t& outHeight) const;

	private:
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
		std::unique_ptr<VulkanTexture> m_WhiteTexture;
		std::unique_ptr<VulkanImGui> m_VulkanImGui;

		glm::mat4 m_SceneViewProjection{ 1.0f };
		bool m_SceneActive = false;
		bool m_ImGuiFrameActive = false;

		uint32_t m_FrameIndex = 0;
		uint32_t m_ImageIndex = 0;
		bool m_ResizeRequested = false;
		bool m_FrameStarted = false;

		float m_ClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	};
}