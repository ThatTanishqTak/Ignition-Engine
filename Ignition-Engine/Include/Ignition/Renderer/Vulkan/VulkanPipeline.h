#pragma once

#include <vulkan/vulkan.h>

#include <string>

namespace Ignition
{
	class VulkanPipeline
	{
	public:
		VulkanPipeline();
		~VulkanPipeline();

		VulkanPipeline(const VulkanPipeline&) = delete;
		VulkanPipeline& operator=(const VulkanPipeline&) = delete;

		void Initialize(VkDevice device, VkFormat colorFormat, const std::string& spirvPath, VkDescriptorSetLayout textureSetLayout);
		void Shutdown();

		bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

		VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

		void Bind(VkCommandBuffer commandBuffer) const;

	private:
		VkShaderModule CreateShaderModule(const std::string& spirvPath) const;

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};
}