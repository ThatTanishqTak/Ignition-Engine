#include "Ignition/Renderer/Vulkan/VulkanLineRenderer.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Core/Utilities.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include <array>
#include <cstddef>
#include <cstring>

namespace Ignition
{
	namespace
	{
		constexpr VkDeviceSize InitialVertexBufferSize = 64 * 1024;
	}

	VulkanLineRenderer::VulkanLineRenderer() = default;
	VulkanLineRenderer::~VulkanLineRenderer() = default;

	void VulkanLineRenderer::Initialize(VkDevice device, VmaAllocator allocator, VkFormat colorFormat, VkFormat depthFormat, const std::string& spirvPath)
	{
		IG_CORE_INFO("------- INITIALIZING DEBUG LINE RENDERER -------");

		m_Device = device;
		m_Allocator = allocator;

		const VkShaderModule shaderModule = CreateShaderModule(spirvPath);

		if (shaderModule == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("Debug line renderer disabled: could not load '{}'", spirvPath);

			return;
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
		bindingDescription.stride = sizeof(DebugLineVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = static_cast<uint32_t>(offsetof(DebugLineVertex, Position));

		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = static_cast<uint32_t>(offsetof(DebugLineVertex, Color));

		VkPipelineVertexInputStateCreateInfo vertexInputState{};
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputState.vertexBindingDescriptionCount = 1;
		vertexInputState.pVertexBindingDescriptions = &bindingDescription;
		vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputState.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
		inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizationState{};
		rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationState.lineWidth = 1.0f;

		VkPipelineMultisampleStateCreateInfo multisampleState{};
		multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo colorBlendState{};
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 1;
		colorBlendState.pAttachments = &colorBlendAttachment;

		VkPipelineDepthStencilStateCreateInfo depthStencilState{};
		depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilState.depthTestEnable = VK_TRUE;
		depthStencilState.depthWriteEnable = VK_FALSE;
		depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

		const std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
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

		bool failed = !VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline));

		if (!failed)
		{
			depthStencilState.depthTestEnable = VK_FALSE;

			failed = !VK_CHECK(vkCreateGraphicsPipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_PipelineOverlay));
		}

		vkDestroyShaderModule(m_Device, shaderModule, nullptr);

		if (failed)
		{
			Shutdown();

			return;
		}

		IG_CORE_INFO("------- DEBUG LINE RENDERER INITIALIZED -------");
	}

	void VulkanLineRenderer::Shutdown()
	{
		for (VulkanBuffer& buffer : m_VertexBuffers)
		{
			if (buffer.IsValid())
			{
				buffer.Shutdown();
			}
		}

		if (m_PipelineOverlay != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(m_Device, m_PipelineOverlay, nullptr);
			m_PipelineOverlay = VK_NULL_HANDLE;
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
	}

	VkShaderModule VulkanLineRenderer::CreateShaderModule(const std::string& spirvPath) const
	{
		const std::vector<char> code = Utilities::CoreUtilities::ReadBinaryFile(spirvPath);

		if (code.empty())
		{
			return VK_NULL_HANDLE;
		}

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule = VK_NULL_HANDLE;

		if (!VK_CHECK(vkCreateShaderModule(m_Device, &createInfo, nullptr, &shaderModule)))
		{
			return VK_NULL_HANDLE;
		}

		return shaderModule;
	}

	bool VulkanLineRenderer::EnsureVertexCapacity(uint32_t frameIndex, VkDeviceSize requiredSize)
	{
		VulkanBuffer& buffer = m_VertexBuffers[frameIndex];

		if (buffer.IsValid() && buffer.GetSize() >= requiredSize)
		{
			return true;
		}

		// The caller already waited on this frame's fence, so nothing in flight can be using it
		if (buffer.IsValid())
		{
			buffer.Shutdown();
		}

		VkDeviceSize size = InitialVertexBufferSize;

		while (size < requiredSize)
		{
			size *= 2;
		}

		buffer.Initialize(m_Allocator, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VulkanBufferAccess::HostWrite);

		return buffer.IsValid();
	}

	void VulkanLineRenderer::DrawExternal(VkCommandBuffer commandBuffer, VkBuffer vertexBuffer, uint32_t vertexCount, const glm::mat4& viewProjection)
	{
		if (!IsValid() || vertexBuffer == VK_NULL_HANDLE || vertexCount == 0)
		{
			return;
		}

		const VkDeviceSize offset = 0;

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjection);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
		vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
	}

	void VulkanLineRenderer::DrawExternalIndirect(VkCommandBuffer commandBuffer, VkBuffer vertexBuffer, VkBuffer indirectBuffer, const glm::mat4& viewProjection)
	{
		if (!IsValid() || vertexBuffer == VK_NULL_HANDLE || indirectBuffer == VK_NULL_HANDLE)
		{
			return;
		}

		const VkDeviceSize offset = 0;

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjection);
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);

		// The vertex count was appended by a compute kernel; only the GPU knows it
		vkCmdDrawIndirect(commandBuffer, indirectBuffer, 0, 1, sizeof(VkDrawIndirectCommand));
	}

	void VulkanLineRenderer::Draw(VkCommandBuffer commandBuffer, uint32_t frameIndex, const glm::mat4& viewProjection, const std::vector<DebugLineVertex>& depthTested, const std::vector<DebugLineVertex>& overlay)
	{
		if (!IsValid() || (depthTested.empty() && overlay.empty()))
		{
			return;
		}

		const VkDeviceSize depthTestedSize = depthTested.size() * sizeof(DebugLineVertex);
		const VkDeviceSize overlaySize = overlay.size() * sizeof(DebugLineVertex);

		if (!EnsureVertexCapacity(frameIndex, depthTestedSize + overlaySize))
		{
			return;
		}

		VulkanBuffer& buffer = m_VertexBuffers[frameIndex];

		if (void* mapped = buffer.Map())
		{
			if (depthTestedSize > 0)
			{
				std::memcpy(mapped, depthTested.data(), depthTestedSize);
			}

			if (overlaySize > 0)
			{
				std::memcpy(static_cast<std::byte*>(mapped) + depthTestedSize, overlay.data(), overlaySize);
			}

			buffer.Unmap();
		}
		else
		{
			return;
		}

		vkCmdPushConstants(commandBuffer, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &viewProjection);

		const VkBuffer vertexBuffer = buffer.GetBuffer();

		if (!depthTested.empty())
		{
			const VkDeviceSize offset = 0;

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(depthTested.size()), 1, 0, 0);
		}

		if (!overlay.empty())
		{
			const VkDeviceSize offset = depthTestedSize;

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineOverlay);
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
			vkCmdDraw(commandBuffer, static_cast<uint32_t>(overlay.size()), 1, 0, 0);
		}
	}
}