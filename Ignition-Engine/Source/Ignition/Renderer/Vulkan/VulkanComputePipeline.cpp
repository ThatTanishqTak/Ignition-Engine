#include "Ignition/Renderer/Vulkan/VulkanComputePipeline.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

namespace Ignition
{
	VulkanComputePipeline::VulkanComputePipeline() = default;
	VulkanComputePipeline::~VulkanComputePipeline() = default;

	void VulkanComputePipeline::Initialize(VkDevice device, VkShaderModule shaderModule, const char* entryPoint, VkDescriptorSetLayout setLayout, uint32_t pushConstantSize)
	{
		m_Device = device;

		if (device == VK_NULL_HANDLE || shaderModule == VK_NULL_HANDLE || !entryPoint)
		{
			IG_CORE_ERROR("Compute pipeline creation aborted: invalid device, module or entry point");

			return;
		}

		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = pushConstantSize;

		VkPipelineLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layoutInfo.setLayoutCount = setLayout != VK_NULL_HANDLE ? 1 : 0;
		layoutInfo.pSetLayouts = setLayout != VK_NULL_HANDLE ? &setLayout : nullptr;
		layoutInfo.pushConstantRangeCount = pushConstantSize > 0 ? 1 : 0;
		layoutInfo.pPushConstantRanges = pushConstantSize > 0 ? &pushConstantRange : nullptr;

		if (!VK_CHECK(vkCreatePipelineLayout(m_Device, &layoutInfo, nullptr, &m_PipelineLayout)))
		{
			m_PipelineLayout = VK_NULL_HANDLE;

			return;
		}

		VkComputePipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		pipelineInfo.stage.module = shaderModule;
		pipelineInfo.stage.pName = entryPoint;
		pipelineInfo.layout = m_PipelineLayout;

		if (!VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline)))
		{
			IG_CORE_ERROR("Compute pipeline creation failed for entry point '{}'", entryPoint);

			m_Pipeline = VK_NULL_HANDLE;

			vkDestroyPipelineLayout(m_Device, m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}
	}

	void VulkanComputePipeline::Shutdown()
	{
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
	}

	void VulkanComputePipeline::Bind(VkCommandBuffer commandBuffer) const
	{
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	}

	void VulkanComputePipeline::Dispatch(VkCommandBuffer commandBuffer, uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ) const
	{
		if (groupsX == 0 || groupsY == 0 || groupsZ == 0)
		{
			return;
		}

		vkCmdDispatch(commandBuffer, groupsX, groupsY, groupsZ);
	}
}