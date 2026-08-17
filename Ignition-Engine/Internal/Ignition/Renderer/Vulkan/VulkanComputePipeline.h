#pragma once

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Ignition
{
	class VulkanComputePipeline
	{
	public:
		VulkanComputePipeline();
		~VulkanComputePipeline();

		VulkanComputePipeline(const VulkanComputePipeline&) = delete;
		VulkanComputePipeline& operator=(const VulkanComputePipeline&) = delete;

		void Initialize(VkDevice device, VkShaderModule shaderModule, const char* entryPoint, VkDescriptorSetLayout setLayout, uint32_t pushConstantSize);
		void Shutdown();

		bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

		void Bind(VkCommandBuffer commandBuffer) const;
		void Dispatch(VkCommandBuffer commandBuffer, uint32_t groupsX, uint32_t groupsY = 1, uint32_t groupsZ = 1) const;

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}