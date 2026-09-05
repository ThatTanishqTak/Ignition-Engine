#include "Ignition/Renderer/Vulkan/VulkanUITextureTable.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Renderer/Vulkan/VulkanFrameContext.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"

#include <algorithm>

namespace Ignition
{
	VulkanUITextureTable::VulkanUITextureTable() = default;
	VulkanUITextureTable::~VulkanUITextureTable() = default;

	void VulkanUITextureTable::Initialize(VkPhysicalDevice physicalDevice, VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VkSampler sampler)
	{
		IG_CORE_INFO("------- INITIALIZING UI TEXTURE TABLE -------");

		m_Device = device;
		m_Sampler = sampler;

		if (m_Device == VK_NULL_HANDLE || m_Sampler == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("UI texture table disabled: no device or sampler");

			return;
		}

		VkPhysicalDeviceDescriptorIndexingProperties indexingProperties{};
		indexingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES;

		VkPhysicalDeviceProperties2 deviceProperties{};
		deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties.pNext = &indexingProperties;

		vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties);

		m_Capacity = std::min({ RequestedCapacity, indexingProperties.maxPerStageDescriptorUpdateAfterBindSampledImages, indexingProperties.maxDescriptorSetUpdateAfterBindSampledImages });

		if (m_Capacity < 2)
		{
			IG_CORE_ERROR("UI texture table disabled: the device reports room for {} update-after-bind sampled images", m_Capacity);

			return;
		}

		VkDescriptorSetLayoutBinding binding{};
		binding.binding = 0;
		binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		binding.descriptorCount = m_Capacity;
		binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		const VkDescriptorBindingFlags bindingFlags = VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT;

		VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{};
		bindingFlagsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		bindingFlagsInfo.bindingCount = 1;
		bindingFlagsInfo.pBindingFlags = &bindingFlags;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo{};
		layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutCreateInfo.pNext = &bindingFlagsInfo;
		layoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
		layoutCreateInfo.bindingCount = 1;
		layoutCreateInfo.pBindings = &binding;

		if (!VK_CHECK(vkCreateDescriptorSetLayout(m_Device, &layoutCreateInfo, nullptr, &m_SetLayout)))
		{
			m_SetLayout = VK_NULL_HANDLE;

			return;
		}

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = m_Capacity;

		VkDescriptorPoolCreateInfo poolCreateInfo{};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
		poolCreateInfo.maxSets = 1;
		poolCreateInfo.poolSizeCount = 1;
		poolCreateInfo.pPoolSizes = &poolSize;

		if (!VK_CHECK(vkCreateDescriptorPool(m_Device, &poolCreateInfo, nullptr, &m_DescriptorPool)))
		{
			m_DescriptorPool = VK_NULL_HANDLE;

			Shutdown();

			return;
		}

		VkDescriptorSetVariableDescriptorCountAllocateInfo variableCountInfo{};
		variableCountInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
		variableCountInfo.descriptorSetCount = 1;
		variableCountInfo.pDescriptorCounts = &m_Capacity;

		VkDescriptorSetAllocateInfo allocateInfo{};
		allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateInfo.pNext = &variableCountInfo;
		allocateInfo.descriptorPool = m_DescriptorPool;
		allocateInfo.descriptorSetCount = 1;
		allocateInfo.pSetLayouts = &m_SetLayout;

		if (!VK_CHECK(vkAllocateDescriptorSets(m_Device, &allocateInfo, &m_DescriptorSet)))
		{
			m_DescriptorSet = VK_NULL_HANDLE;

			Shutdown();

			return;
		}

		if (!CreateWhiteSlot(graphicsQueue, graphicsQueueFamily, allocator))
		{
			IG_CORE_ERROR("UI texture table disabled: the white fallback could not be created");

			Shutdown();

			return;
		}

		IG_CORE_INFO("------- UI TEXTURE TABLE INITIALIZED ({} SLOTS) -------", m_Capacity);
	}

	bool VulkanUITextureTable::CreateWhiteSlot(VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator)
	{
		constexpr uint8_t whitePixel[4] = { 255, 255, 255, 255 };

		m_WhiteImage.Initialize(m_Device, allocator, 1, 1, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		if (!m_WhiteImage.IsValid())
		{
			return false;
		}

		const bool uploaded = Utilities::VulkanUtilities::UploadViaStaging(m_Device, graphicsQueue, graphicsQueueFamily, allocator, whitePixel, sizeof(whitePixel), [this](VkCommandBuffer commandBuffer, const VulkanBuffer& staging)
		{
			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_WhiteImage.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

			VkBufferImageCopy region{};
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = { 1, 1, 1 };

			vkCmdCopyBufferToImage(commandBuffer, staging.GetBuffer(), m_WhiteImage.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_WhiteImage.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
		});

		if (!uploaded)
		{
			return false;
		}

		// Every slot starts pointing at the white pixel, so a primitive naming a stale index samples 1 rather than garbage
		for (uint32_t slot = 0; slot < m_Capacity; ++slot)
		{
			Write(slot, m_WhiteImage.GetImageView());
		}

		return true;
	}

	void VulkanUITextureTable::Shutdown()
	{
		m_PendingReleases.clear();
		m_FreeSlots.clear();

		if (m_WhiteImage.IsValid())
		{
			m_WhiteImage.Shutdown();
		}

		if (m_DescriptorPool != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorPool(m_Device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
			m_DescriptorSet = VK_NULL_HANDLE;
		}

		if (m_SetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(m_Device, m_SetLayout, nullptr);
			m_SetLayout = VK_NULL_HANDLE;
		}

		m_Device = VK_NULL_HANDLE;
		m_Sampler = VK_NULL_HANDLE;
		m_NextSlot = 1;
	}

	void VulkanUITextureTable::Write(uint32_t slot, VkImageView imageView)
	{
		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = m_Sampler;
		imageInfo.imageView = imageView;
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.dstSet = m_DescriptorSet;
		write.dstBinding = 0;
		write.dstArrayElement = slot;
		write.descriptorCount = 1;
		write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Device, 1, &write, 0, nullptr);
	}

	uint32_t VulkanUITextureTable::Acquire(VkImageView imageView)
	{
		if (!IsValid() || imageView == VK_NULL_HANDLE)
		{
			return 0;
		}

		uint32_t slot = 0;

		if (!m_FreeSlots.empty())
		{
			slot = m_FreeSlots.back();
			m_FreeSlots.pop_back();
		}
		else if (m_NextSlot < m_Capacity)
		{
			slot = m_NextSlot++;
		}
		else
		{
			IG_CORE_ERROR("UI texture table is full at {} slots, falling back to white", m_Capacity);

			return 0;
		}

		Write(slot, imageView);

		return slot;
	}

	void VulkanUITextureTable::Release(uint32_t slot, uint64_t frameNumber)
	{
		if (slot == 0 || slot >= m_Capacity)
		{
			return;
		}

		m_PendingReleases.push_back(PendingRelease{ slot, frameNumber });
	}

	void VulkanUITextureTable::ProcessRetirement(uint64_t frameNumber)
	{
		while (!m_PendingReleases.empty() && m_PendingReleases.front().FrameNumber + VulkanFrameContext::MaximumFramesInFlight <= frameNumber)
		{
			const uint32_t slot = m_PendingReleases.front().Slot;
			m_PendingReleases.pop_front();

			// Back to white before it goes on the free list, so nothing can sample a destroyed view through a stale index
			Write(slot, m_WhiteImage.GetImageView());

			m_FreeSlots.push_back(slot);
		}
	}
}