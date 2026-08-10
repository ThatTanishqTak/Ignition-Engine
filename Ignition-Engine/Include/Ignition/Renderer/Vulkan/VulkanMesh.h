#pragma once

#include "Ignition/Renderer/Vertex.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"

#include <vulkan/vulkan.h>

#include <cstdint>
#include <vector>

namespace Ignition
{
	class VulkanMesh
	{
	public:
		VulkanMesh();
		~VulkanMesh();

		VulkanMesh(const VulkanMesh&) = delete;
		VulkanMesh& operator=(const VulkanMesh&) = delete;

		void Initialize(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
		void Shutdown();

		bool IsValid() const { return m_IndexCount > 0; }

		void Bind(VkCommandBuffer commandBuffer) const;
		uint32_t GetIndexCount() const { return m_IndexCount; }

	private:
		VulkanBuffer m_VertexBuffer;
		VulkanBuffer m_IndexBuffer;
		uint32_t m_IndexCount = 0;
	};
}