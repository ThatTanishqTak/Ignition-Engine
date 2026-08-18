#pragma once

#include <vulkan/vulkan.h>

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

namespace Ignition
{
	class VulkanGPUTimer;
	class VulkanLineRenderer;

	// A textured quad a compute pass wants drawn inside the scene - the fluid slice plane
	struct VulkanSceneQuad
	{
		glm::mat4 Transform{ 1.0f };
		VkDescriptorSet Texture = VK_NULL_HANDLE; // texture-layout set, sampled by the mesh pipeline
		glm::vec4 Tint{ 1.0f };
	};

	class VulkanComputePass
	{
	public:
		virtual ~VulkanComputePass() = default;

		virtual void RecordCompute(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanGPUTimer* timer) = 0;
		virtual bool GetSceneQuad(VulkanSceneQuad& quad) { (void)quad; return false; }
		virtual void RecordSceneLines(VkCommandBuffer commandBuffer, uint32_t frameIndex, VulkanLineRenderer& lines, const glm::mat4& viewProjection) { (void)commandBuffer; (void)frameIndex; (void)lines; (void)viewProjection; }
	};
}