#pragma once

#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"

#include <vulkan/vulkan.h>

#include <array>
#include <string>

namespace Ignition
{
	class VulkanDescriptorAllocator;

	namespace UI
	{
		class DrawList;
	}

	class VulkanUIRenderer
	{
	public:
		VulkanUIRenderer();
		~VulkanUIRenderer();

		VulkanUIRenderer(const VulkanUIRenderer&) = delete;
		VulkanUIRenderer& operator=(const VulkanUIRenderer&) = delete;

		void Initialize(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, const std::string& spirvPath, VkDescriptorSetLayout textureSetLayout, VulkanDescriptorAllocator& descriptorAllocator);
		void Shutdown();

		bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

		void FlushUploads(VkCommandBuffer commandBuffer);

		void Draw(VkCommandBuffer commandBuffer, uint32_t frameIndex, const UI::DrawList& drawList, VkExtent2D targetExtent, bool viewportFlipped, VkDescriptorSet textureSet);

	private:
		bool EnsureCapacity(uint32_t frameIndex, VkDeviceSize vertexSize, VkDeviceSize indexSize, VkDeviceSize primitiveSize);
		bool Grow(VulkanBuffer& buffer, VkDeviceSize requiredSize, VkBufferUsageFlags usage, VkDeviceSize initialSize);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VmaAllocator m_Allocator = VK_NULL_HANDLE;
		VulkanDescriptorAllocator* m_DescriptorAllocator = nullptr;

		VkDescriptorSetLayout m_PrimitiveSetLayout = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_VertexBuffers;
		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_IndexBuffers;
		std::array<VulkanBuffer, VulkanFrameContext::MaximumFramesInFlight> m_PrimitiveBuffers;
		std::array<VkDescriptorSet, VulkanFrameContext::MaximumFramesInFlight> m_PrimitiveSets{};
	};
}