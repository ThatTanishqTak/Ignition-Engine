#pragma once

#include "Ignition/Renderer/DebugDrawBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>

#include <array>
#include <string>
#include <vector>

namespace Ignition
{
	class VulkanLineRenderer
	{
	public:
		VulkanLineRenderer();
		~VulkanLineRenderer();

		VulkanLineRenderer(const VulkanLineRenderer&) = delete;
		VulkanLineRenderer& operator=(const VulkanLineRenderer&) = delete;

		void Initialize(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, const std::string& spirvPath);
		void Shutdown();

		bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE && m_PipelineOverlay != VK_NULL_HANDLE; }

		void Draw(VkCommandBuffer commandBuffer, uint32_t frameIndex, const glm::mat4& viewProjection, const std::vector<DebugLineVertex>& depthTested, const std::vector<DebugLineVertex>& overlay);

		// GPU-resident line lists (compute-written vertex buffers in DebugLineVertex layout) - nothing crosses back to the CPU
		void DrawExternal(VkCommandBuffer commandBuffer, VkBuffer vertexBuffer, uint32_t vertexCount, const glm::mat4& viewProjection);
		void DrawExternalIndirect(VkCommandBuffer commandBuffer, VkBuffer vertexBuffer, VkBuffer indirectBuffer, const glm::mat4& viewProjection);

	private:
		VkShaderModule CreateShaderModule(const std::string& spirvPath) const;
		bool EnsureVertexCapacity(uint32_t frameIndex, VkDeviceSize requiredSize);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE; // depth-tested
		VkPipeline m_PipelineOverlay = VK_NULL_HANDLE; // always on top

		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_VertexBuffers;
	};
}