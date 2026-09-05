#include "Ignition/Renderer/Vulkan/VulkanUIRenderer.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/UI/DrawList.h"

#include <glm/mat4x4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>

namespace Ignition
{
	namespace
	{
		constexpr VkDeviceSize InitialVertexBufferSize = 256 * 1024;
		constexpr VkDeviceSize InitialIndexBufferSize = 128 * 1024;
		constexpr VkDeviceSize InitialPrimitiveBufferSize = 64 * 1024;
	}

	VulkanUIRenderer::VulkanUIRenderer() = default;
	VulkanUIRenderer::~VulkanUIRenderer() = default;

	void VulkanUIRenderer::Initialize(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, const std::string& spirvPath, VkDescriptorSetLayout textureSetLayout, VulkanDescriptorAllocator& descriptorAllocator)
	{
		IG_CORE_INFO("------- INITIALIZING UI RENDERER -------");

		m_Device = device;
		m_Allocator = allocator;
		m_DescriptorAllocator = &descriptorAllocator;

		if (textureSetLayout == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("UI renderer disabled: the texture table has no set layout");

			return;
		}

		const VkShaderModule shaderModule = Utilities::VulkanUtilities::CreateShaderModule(m_Device, spirvPath);

		if (shaderModule == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("UI renderer disabled: could not load '{}'", spirvPath);

			return;
		}

		VkDescriptorSetLayoutBinding primitiveBinding{};
		primitiveBinding.binding = 0;
		primitiveBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		primitiveBinding.descriptorCount = 1;
		primitiveBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

		m_PrimitiveSetLayout = m_DescriptorAllocator->CreateSetLayout({ primitiveBinding });

		if (m_PrimitiveSetLayout == VK_NULL_HANDLE)
		{
			vkDestroyShaderModule(m_Device, shaderModule, nullptr);

			return;
		}

		for (VkDescriptorSet& primitiveSet : m_PrimitiveSets)
		{
			primitiveSet = m_DescriptorAllocator->Allocate(m_PrimitiveSetLayout);
		}

		std::array<VkPipelineShaderStageCreateInfo, 2> stages{};

		stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stages[0].module = shaderModule;
		stages[0].pName = "vertexMain";

		stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stages[1].module = shaderModule;
		stages[1].pName = "fragmentMain";

		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(UI::DrawVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = static_cast<uint32_t>(offsetof(UI::DrawVertex, Position));

		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[1].offset = static_cast<uint32_t>(offsetof(UI::DrawVertex, UV));

		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = static_cast<uint32_t>(offsetof(UI::DrawVertex, Color));

		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
		attributeDescriptions[3].offset = static_cast<uint32_t>(offsetof(UI::DrawVertex, Primitive));

		VkPipelineVertexInputStateCreateInfo vertexInputState{};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE; // a flipped ortho would otherwise cull every triangle
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Premultiplied: the fragment already multiplied rgb by alpha, so the source factor is ONE
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_FALSE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;

		const std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);

		const std::array<VkDescriptorSetLayout, 2> setLayouts = { m_PrimitiveSetLayout, textureSetLayout };

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		layoutInfo.pSetLayouts = setLayouts.data();
		layoutInfo.pushConstantRangeCount = 1;
		layoutInfo.pPushConstantRanges = &pushConstantRange;

		if (!VK_CHECK(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &m_PipelineLayout)))
		{
			vkDestroyShaderModule(m_Device, shaderModule, nullptr);

			return;
		}

		VkPipelineRenderingCreateInfo renderingCreateInfo{};
		renderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingCreateInfo.colorAttachmentCount = 1;
		renderingCreateInfo.pColorAttachmentFormats = &colorFormat;
		renderingCreateInfo.depthAttachmentFormat = depthFormat;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingCreateInfo;
		pipelineInfo.stageCount = static_cast<uint32_t>(stages.size());
		pipelineInfo.pStages = stages.data();
		pipelineInfo.pVertexInputState = &vertexInputState;
		pipelineInfo.pInputAssemblyState = &inputAssemblyState;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizationState;
		pipelineInfo.pMultisampleState = &multisampleState;
		pipelineInfo.pDepthStencilState = &depthStencilState;
		pipelineInfo.pColorBlendState = &colorBlendState;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;

		const bool failed = !VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

		vkDestroyShaderModule(m_Device, shaderModule, nullptr);

		if (failed)
		{
			Shutdown();

			return;
		}

		IG_CORE_INFO("------- UI RENDERER INITIALIZED -------");
	}

	void VulkanUIRenderer::Shutdown()
	{
		for (VulkanBuffer& buffer : m_VertexBuffers)
		{
			if (buffer.IsValid())
			{
				buffer.Shutdown();
			}
		}

		for (VulkanBuffer& buffer : m_IndexBuffers)
		{
			if (buffer.IsValid())
			{
				buffer.Shutdown();
			}
		}

		for (VulkanBuffer& buffer : m_PrimitiveBuffers)
		{
			if (buffer.IsValid())
			{
				buffer.Shutdown();
			}
		}

		if (m_DescriptorAllocator)
		{
			for (VkDescriptorSet& primitiveSet : m_PrimitiveSets)
			{
				m_DescriptorAllocator->Free(primitiveSet);
				primitiveSet = VK_NULL_HANDLE;
			}

			if (m_PrimitiveSetLayout != VK_NULL_HANDLE)
			{
				m_DescriptorAllocator->DestroySetLayout(m_PrimitiveSetLayout);
				m_PrimitiveSetLayout = VK_NULL_HANDLE;
			}
		}

		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_Device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;
		m_Allocator = VK_NULL_HANDLE;
		m_DescriptorAllocator = nullptr;
	}

	void VulkanUIRenderer::FlushUploads(VkCommandBuffer commandBuffer)
	{
		(void)commandBuffer;

		// TODO(Phase 2): drain the glyph atlas staging queue here. The call site is load-bearing on its own - it is the
		// only point in the frame that sits outside every dynamic rendering block
	}

	bool VulkanUIRenderer::Grow(VulkanBuffer& buffer, VkDeviceSize requiredSize, VkBufferUsageFlags usage, VkDeviceSize initialSize)
	{
		if (buffer.IsValid() && buffer.GetSize() >= requiredSize)
		{
			return true;
		}

		// The caller already waited on this frame's fence, so nothing in flight can be using it
		if (buffer.IsValid())
		{
			buffer.Shutdown();
		}

		VkDeviceSize size = initialSize;

		while (size < requiredSize)
		{
			size *= 2;
		}

		buffer.Initialize(m_Allocator, size, usage, VulkanBufferAccess::HostWrite);

		return buffer.IsValid();
	}

	bool VulkanUIRenderer::EnsureCapacity(uint32_t frameIndex, VkDeviceSize vertexSize, VkDeviceSize indexSize, VkDeviceSize primitiveSize)
	{
		const bool primitivesGrew = !m_PrimitiveBuffers[frameIndex].IsValid() || m_PrimitiveBuffers[frameIndex].GetSize() < primitiveSize;

		if (!Grow(m_VertexBuffers[frameIndex], vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, InitialVertexBufferSize))
		{
			return false;
		}

		if (!Grow(m_IndexBuffers[frameIndex], indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, InitialIndexBufferSize))
		{
			return false;
		}

		if (!Grow(m_PrimitiveBuffers[frameIndex], primitiveSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, InitialPrimitiveBufferSize))
		{
			return false;
		}

		if (primitivesGrew && m_PrimitiveSets[frameIndex] != VK_NULL_HANDLE)
		{
			VulkanDescriptorAllocator::WriteStorageBuffer(m_Device, m_PrimitiveSets[frameIndex], 0, m_PrimitiveBuffers[frameIndex].GetBuffer(), m_PrimitiveBuffers[frameIndex].GetSize());
		}

		return m_PrimitiveSets[frameIndex] != VK_NULL_HANDLE;
	}

	void VulkanUIRenderer::Draw(VkCommandBuffer commandBuffer, uint32_t frameIndex, const UI::DrawList& drawList, VkExtent2D targetExtent, bool viewportFlipped, VkDescriptorSet textureSet)
	{
		if (!IsValid() || drawList.IsEmpty() || textureSet == VK_NULL_HANDLE || targetExtent.width == 0 || targetExtent.height == 0)
		{
			return;
		}

		const VkDeviceSize vertexSize = drawList.GetVertices().size() * sizeof(UI::DrawVertex);
		const VkDeviceSize indexSize = drawList.GetIndices().size() * sizeof(uint32_t);
		const VkDeviceSize primitiveSize = std::max<VkDeviceSize>(drawList.GetPrimitives().size() * sizeof(UI::DrawPrimitive), sizeof(UI::DrawPrimitive));

		if (!EnsureCapacity(frameIndex, vertexSize, indexSize, primitiveSize))
		{
			return;
		}

		if (void* mapped = m_VertexBuffers[frameIndex].Map())
		{
			std::memcpy(mapped, drawList.GetVertices().data(), vertexSize);
			m_VertexBuffers[frameIndex].Unmap();
		}
		else
		{
			return;
		}

		if (void* mapped = m_IndexBuffers[frameIndex].Map())
		{
			std::memcpy(mapped, drawList.GetIndices().data(), indexSize);
			m_IndexBuffers[frameIndex].Unmap();
		}
		else
		{
			return;
		}

		if (void* mapped = m_PrimitiveBuffers[frameIndex].Map())
		{
			if (!drawList.GetPrimitives().empty())
			{
				std::memcpy(mapped, drawList.GetPrimitives().data(), drawList.GetPrimitives().size() * sizeof(UI::DrawPrimitive));
			}

			m_PrimitiveBuffers[frameIndex].Unmap();
		}
		else
		{
			return;
		}

		const float width = static_cast<float>(targetExtent.width);
		const float height = static_cast<float>(targetExtent.height);

		// Surface pixels straight to clip space, with the pass's viewport flip carried by one sign rather than by a
		// second viewport - the same flip Fluid3D.slang documents for the slice quad's row 0
		glm::mat4 projection(1.0f);
		projection[0][0] = 2.0f / width;
		projection[1][1] = (viewportFlipped ? -2.0f : 2.0f) / height;
		projection[3][0] = -1.0f;
		projection[3][1] = viewportFlipped ? 1.0f : -1.0f;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = viewportFlipped ? height : 0.0f;
		viewport.width = width;
		viewport.height = viewportFlipped ? -height : height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		const std::array<VkDescriptorSet, 2> descriptorSets = { m_PrimitiveSets[frameIndex], textureSet };
		const VkBuffer vertexBuffer = m_VertexBuffers[frameIndex].GetBuffer();
		const VkDeviceSize vertexOffset = 0;

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, static_cast<uint32_t>(descriptorSets.size()), descriptorSets.data(), 0, nullptr);
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &projection);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &vertexOffset);
		vkCmdBindIndexBuffer(commandBuffer, m_IndexBuffers[frameIndex].GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		for (const UI::DrawCommand& command : drawList.GetCommands())
		{
			if (command.IndexCount == 0)
			{
				continue;
			}

			// Scissor is in framebuffer space whichever way the viewport points, and Vulkan rejects a negative offset
			const int32_t left = std::max(static_cast<int32_t>(command.ClipRect.GetLeft()), 0);
			const int32_t top = std::max(static_cast<int32_t>(command.ClipRect.GetTop()), 0);
			const int32_t right = std::min(static_cast<int32_t>(command.ClipRect.GetRight() + 0.5f), static_cast<int32_t>(targetExtent.width));
			const int32_t bottom = std::min(static_cast<int32_t>(command.ClipRect.GetBottom() + 0.5f), static_cast<int32_t>(targetExtent.height));

			if (right <= left || bottom <= top)
			{
				continue;
			}

			VkRect2D scissor{};
			scissor.offset = { left, top };
			scissor.extent = { static_cast<uint32_t>(right - left), static_cast<uint32_t>(bottom - top) };

			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			vkCmdDrawIndexed(commandBuffer, command.IndexCount, 1, command.IndexOffset, 0, 0);
		}

		// Hand the pass back a full-target scissor so nothing recorded after this inherits the last clip rect
		VkRect2D full{};
		full.offset = { 0, 0 };
		full.extent = targetExtent;

		vkCmdSetScissor(commandBuffer, 0, 1, &full);
	}
}