#include "Ignition/Renderer/Vulkan/VulkanMesh.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <cstring>

namespace Ignition
{
	namespace
	{

		bool UploadDeviceLocalBuffer(VulkanBuffer& outBuffer, VkDevice device, VkQueue queue, uint32_t queueFamily, VmaAllocator allocator, const void* data, VkDeviceSize size, VkBufferUsageFlags usage)
		{
			VulkanBuffer stagingBuffer;
			stagingBuffer.Initialize(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);

			if (!stagingBuffer.IsValid())
			{
				return false;
			}

			void* mapped = stagingBuffer.Map();

			if (!mapped)
			{
				stagingBuffer.Shutdown();

				return false;
			}

			std::memcpy(mapped, data, static_cast<size_t>(size));
			stagingBuffer.Unmap();

			outBuffer.Initialize(allocator, size, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT, false);

			if (!outBuffer.IsValid())
			{
				stagingBuffer.Shutdown();

				return false;
			}

			const bool copied = Utilities::VulkanUtilities::SubmitOneShotCommands(device, queue, queueFamily, [&](VkCommandBuffer commandBuffer)
			{
				VkBufferCopy copyRegion{};
				copyRegion.size = size;

				vkCmdCopyBuffer(commandBuffer, stagingBuffer.GetBuffer(), outBuffer.GetBuffer(), 1, &copyRegion);
			});

			stagingBuffer.Shutdown();

			if (!copied)
			{
				outBuffer.Shutdown();

				return false;
			}

			return true;
		}
	}

	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

	VulkanMesh::VulkanMesh() = default;
	VulkanMesh::~VulkanMesh() = default;

	void VulkanMesh::Initialize(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN MESH -------");

		if (vertices.empty() || indices.empty())
		{
			IG_CORE_ERROR("Mesh creation failed: empty vertex or index data");

			return;
		}

		const VkDeviceSize vertexSize = static_cast<VkDeviceSize>(vertices.size() * sizeof(Vertex));
		const VkDeviceSize indexSize = static_cast<VkDeviceSize>(indices.size() * sizeof(uint32_t));

		if (!UploadDeviceLocalBuffer(m_VertexBuffer, device, graphicsQueue, graphicsQueueFamily, allocator, vertices.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
		{
			IG_CORE_ERROR("Mesh creation failed: vertex buffer upload");

			return;
		}

		if (!UploadDeviceLocalBuffer(m_IndexBuffer, device, graphicsQueue, graphicsQueueFamily, allocator, indices.data(), indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
		{
			IG_CORE_ERROR("Mesh creation failed: index buffer upload");

			m_VertexBuffer.Shutdown();

			return;
		}

		m_IndexCount = static_cast<uint32_t>(indices.size());

		IG_CORE_INFO("------- VULKAN MESH INITIALIZED -------");
	}

	void VulkanMesh::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN MESH -------");

		m_VertexBuffer.Shutdown();
		m_IndexBuffer.Shutdown();
		m_IndexCount = 0;

		IG_CORE_INFO("------- VULKAN MESH SHUTDOWN COMPLETE -------");
	}

	void VulkanMesh::Bind(VkCommandBuffer commandBuffer) const
	{
		//IG_CORE_TRACE("Binding Command Buffer To Vertex Buffer And Index Buffer");

		const VkBuffer vertexBuffer = m_VertexBuffer.GetBuffer();
		const VkDeviceSize offset = 0;

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffer.GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		//IG_CORE_TRACE("Vertex Buffer And Index Buffer Binded To Command Buffer");
	}
}