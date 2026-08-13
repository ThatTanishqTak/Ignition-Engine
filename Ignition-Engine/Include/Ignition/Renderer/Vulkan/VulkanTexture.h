#pragma once

#include "Ignition/Renderer/Vulkan/VulkanImage.h"

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace Ignition
{
	class VulkanDescriptorAllocator;

	class VulkanTexture
	{
	public:
		VulkanTexture();
		~VulkanTexture();

		VulkanTexture(const VulkanTexture&) = delete;
		VulkanTexture& operator=(const VulkanTexture&) = delete;

		void Initialize(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const void* pixels, uint32_t width, uint32_t height);
		void InitializeFromFile(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& filepath);
		void InitializeFromMemory(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const void* data, size_t size);
		void Shutdown();

		bool IsValid() const { return m_DescriptorSet != VK_NULL_HANDLE; }

		VkDescriptorSet GetDescriptorSet() const { return m_DescriptorSet; }

	private:
		VkDevice m_Device = VK_NULL_HANDLE;
		VulkanDescriptorAllocator* m_DescriptorAllocator = nullptr;
		VulkanImage m_Image;
		VkSampler m_Sampler = VK_NULL_HANDLE;
		VkDescriptorSet m_DescriptorSet = VK_NULL_HANDLE;
	};
}