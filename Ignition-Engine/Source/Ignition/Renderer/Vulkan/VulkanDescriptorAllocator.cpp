#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"

#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

namespace Ignition
{
	namespace
	{
		constexpr uint32_t MaximumTextureSets = 64;
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

		if (Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorSetLayout(m_Device, &layoutCreateInfo, nullptr, &m_TextureSetLayout), "Failed vkCreateDescriptorSetLayout"))
		{
			m_TextureSetLayout = VK_NULL_HANDLE;

			return;
		}

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = MaximumTextureSets;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		poolCreateInfo.maxSets = MaximumTextureSets;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;

		if (Utilities::VulkanUtilities::VKCheck(vkCreateDescriptorPool(m_Device, &poolCreateInfo, nullptr, &m_DescriptorPool), "Failed vkCreateDescriptorPool"))
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

		if (m_TextureSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_TextureSetLayout, nullptr);
			m_TextureSetLayout = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;

		IG_CORE_INFO("------- VULKAN DESCRIPTOR ALLOCATOR SHUTDOWN COMPLETE -------");
	}

	VkDescriptorSet VulkanDescriptorAllocator::Allocate()
	{
		if (!IsValid())
		{
			return VK_NULL_HANDLE;
		}

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_TextureSetLayout;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		if (Utilities::VulkanUtilities::VKCheck(vkAllocateDescriptorSets(m_Device, &allocateInfo, &descriptorSet), "Failed vkAllocateDescriptorSets"))
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
}