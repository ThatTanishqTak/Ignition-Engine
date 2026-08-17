#pragma once

#include <vulkan/vulkan.h>

#include <vector>

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

		// Owned by the allocator: destroyed at Shutdown, or earlier through DestroySetLayout
		VkDescriptorSetLayout CreateSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings);
		void DestroySetLayout(VkDescriptorSetLayout setLayout);

		VkDescriptorSet Allocate();
		VkDescriptorSet Allocate(VkDescriptorSetLayout setLayout);
		void Free(VkDescriptorSet descriptorSet);

		static void WriteStorageBuffer(VkDevice device, VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDeviceSize offset = 0);
		static void WriteStorageImage(VkDevice device, VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView);

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VkDescriptorSetLayout m_TextureSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool m_DescriptorPool = VK_NULL_HANDLE;

		std::vector<VkDescriptorSetLayout> m_OwnedSetLayouts;
	};
}