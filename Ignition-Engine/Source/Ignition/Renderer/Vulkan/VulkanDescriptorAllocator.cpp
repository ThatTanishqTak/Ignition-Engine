#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#include <algorithm>
#include <array>

namespace Ignition
{
	namespace
	{
		constexpr uint32_t MaximumTextureSets = 64;
		constexpr uint32_t MaximumComputeSets = 16;
		constexpr uint32_t MaximumStorageBuffers = 256;
		constexpr uint32_t MaximumStorageImages = 32;
	}

	VulkanDescriptorAllocator::VulkanDescriptorAllocator() = default;
	VulkanDescriptorAllocator::~VulkanDescriptorAllocator() = default;

	void VulkanDescriptorAllocator::Initialize(VkDevice device)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN DESCRIPTOR ALLOCATOR -------");

		m_Device = device;

		VkDescriptorSetLayoutBinding textureBinding{};
		textureBinding.binding = 0;
		textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureBinding.descriptorCount = 1;
		textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = &textureBinding;

		if (!VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutCreateInfo, nullptr, &m_TextureSetLayout)))
		{
			m_TextureSetLayout = VK_NULL_HANDLE;

			return;
		}

		const std::array<VkDescriptorPoolSize, 3> poolSizes = { {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MaximumTextureSets },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MaximumStorageBuffers },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MaximumStorageImages },
		} };

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.maxSets = MaximumTextureSets + MaximumComputeSets;
		poolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolCreateInfo.pPoolSizes = poolSizes.data();

		if (!VK_CHECK(vkCreateDescriptorPool(m_Device, &poolCreateInfo, nullptr, &m_DescriptorPool)))
		{
			m_DescriptorPool = VK_NULL_HANDLE;

			vkDestroyDescriptorSetLayout(m_Device, m_TextureSetLayout, nullptr);
			m_TextureSetLayout = VK_NULL_HANDLE;

			return;
		}

		IG_CORE_INFO("------- VULKAN DESCRIPTOR ALLOCATOR INITIALIZED -------");
	}

	void VulkanDescriptorAllocator::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN VULKAN DESCRIPTOR ALLOCATOR -------");

		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}

		for (VkDescriptorSetLayout setLayout : m_OwnedSetLayouts)
		{
			vkDestroyDescriptorSetLayout(m_Device, setLayout, nullptr);
		}

		m_OwnedSetLayouts.clear();

		if (m_TextureSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_TextureSetLayout, nullptr);
			m_TextureSetLayout = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;

		IG_CORE_INFO("------- VULKAN DESCRIPTOR ALLOCATOR SHUTDOWN COMPLETE -------");
	}

	VkDescriptorSetLayout VulkanDescriptorAllocator::CreateSetLayout(const std::vector<VkDescriptorSetLayoutBinding>& bindings)
	{
		if (m_Device == VK_NULL_HANDLE || bindings.empty())
		{
			return VK_NULL_HANDLE;
		}

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutCreateInfo.pBindings = bindings.data();

		VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;

		if (!VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutCreateInfo, nullptr, &setLayout)))
		{
			return VK_NULL_HANDLE;
		}

		m_OwnedSetLayouts.push_back(setLayout);

		return setLayout;
	}

	void VulkanDescriptorAllocator::DestroySetLayout(VkDescriptorSetLayout setLayout)
	{
		const auto entry = std::find(m_OwnedSetLayouts.begin(), m_OwnedSetLayouts.end(), setLayout);

		if (entry == m_OwnedSetLayouts.end())
		{
			return;
		}

		vkDestroyDescriptorSetLayout(m_Device, setLayout, nullptr);
		m_OwnedSetLayouts.erase(entry);
	}

	VkDescriptorSet VulkanDescriptorAllocator::Allocate()
	{
		return Allocate(m_TextureSetLayout);
	}

	VkDescriptorSet VulkanDescriptorAllocator::Allocate(VkDescriptorSetLayout setLayout)
	{
		if (!IsValid() || setLayout == VK_NULL_HANDLE)
		{
			return VK_NULL_HANDLE;
		}

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &setLayout;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		if (!VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocateInfo, &descriptorSet)))
		{
			return VK_NULL_HANDLE;
		}

		return descriptorSet;
	}

	void VulkanDescriptorAllocator::Free(VkDescriptorSet descriptorSet)
	{
		if (descriptorSet != VK_NULL_HANDLE && m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkFreeDescriptorSets(m_Device, m_DescriptorPool, 1, &descriptorSet);
		}
	}

	void VulkanDescriptorAllocator::WriteStorageBuffer(VkDevice device, VkDescriptorSet descriptorSet, uint32_t binding, VkBuffer buffer, VkDeviceSize range, VkDeviceSize offset)
	{
		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = buffer;
		bufferInfo.offset = offset;
		bufferInfo.range = range;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptorSet;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		write.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanDescriptorAllocator::WriteStorageImage(VkDevice device, VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptorSet;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}

	void VulkanDescriptorAllocator::WriteCombinedImageSampler(VkDevice device, VkDescriptorSet descriptorSet, uint32_t binding, VkImageView imageView, VkSampler sampler)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = sampler;
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = descriptorSet;
		write.dstBinding = binding;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
	}
}