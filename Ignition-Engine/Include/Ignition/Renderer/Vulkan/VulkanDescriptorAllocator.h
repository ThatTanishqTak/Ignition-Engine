#pragma once

#include <vulkan/vulkan.h>

namespace Ignition
{
	class VulkanDescriptorAllocator
	{
	public:
		VulkanDescriptorAllocator();
		~VulkanDescriptorAllocator();

		VulkanDescriptorAllocator(const VulkanDescriptorAllocator&) = delete;
		VulkanDescriptorAllocator& operator=(const VulkanDescriptorAllocator&) = delete;

		void Initialize(VkDevice device);
		void Shutdown();

		bool IsValid() const { return m_DescriptorPool != VK_NULL_HANDLE; }

		VkDescriptorSetLayout GetTextureSetLayout() const { return m_TextureSetLayout; }

		VkDescriptorSet Allocate();
		void Free(VkDescriptorSet descriptorSet);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_TextureSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;
	};
}