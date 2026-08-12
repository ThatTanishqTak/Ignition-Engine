#include "Ignition/Renderer/Vulkan/VulkanTexture.h"

#include "Ignition/Renderer/Vulkan/VulkanDescriptorAllocator.h"
#include "Ignition/Renderer/Vulkan/VulkanBuffer.h"
#include "Ignition/Renderer/Vulkan/Utilities/VulkanUtilities.h"
#include "Ignition/Core/Log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <cstring>

namespace Ignition
{
	VulkanTexture::VulkanTexture() = default;
	VulkanTexture::~VulkanTexture() = default;

	void VulkanTexture::Initialize(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const void* pixels, uint32_t width, uint32_t height)
	{
		IG_CORE_INFO("------- INITIALIZING VULKAN TEXTURE ({}x{}) -------", width, height);

		m_Device = device;
		m_DescriptorAllocator = &descriptorAllocator;

		if (pixels == nullptr || width == 0 || height == 0)
		{
			IG_CORE_ERROR("Vulkan texture initialization aborted: no pixel data");

			return;
		}

		m_Image.Initialize(device, allocator, width, height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);

		if (!m_Image.IsValid())
		{
			IG_CORE_ERROR("Vulkan texture initialization aborted: no image");

			Shutdown();

			return;
		}

		const VkDeviceSize size = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height) * 4;

		VulkanBuffer stagingBuffer;
		stagingBuffer.Initialize(allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true);

		if (!stagingBuffer.IsValid())
		{
			IG_CORE_ERROR("Vulkan texture initialization aborted: no staging buffer");

			Shutdown();

			return;
		}

		void* mapped = stagingBuffer.Map();

		if (!mapped)
		{
			stagingBuffer.Shutdown();
			Shutdown();

			return;
		}

		std::memcpy(mapped, pixels, static_cast<size_t>(size));
		stagingBuffer.Unmap();

		const bool uploaded = Utilities::VulkanUtilities::SubmitOneShotCommands(device, graphicsQueue, graphicsQueueFamily, [&](VkCommandBuffer commandBuffer)
		{
			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_Image.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, 0, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

			VkBufferImageCopy copyRegion{};
			copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.imageSubresource.mipLevel = 0;
			copyRegion.imageSubresource.baseArrayLayer = 0;
			copyRegion.imageSubresource.layerCount = 1;
			copyRegion.imageExtent = { width, height, 1 };

			vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.GetBuffer(), m_Image.GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

			Utilities::VulkanUtilities::TransitionImageLayout(commandBuffer, m_Image.GetImage(), VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_PIPELINE_STAGE_2_COPY_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
		});

		stagingBuffer.Shutdown();

		if (!uploaded)
		{
			IG_CORE_ERROR("Vulkan texture initialization aborted: upload failed");

			Shutdown();

			return;
		}

		VkSamplerCreateInfo samplerCreateInfo{};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		if (Utilities::VulkanUtilities::VKCheck(vkCreateSampler(m_Device, &samplerCreateInfo, nullptr, &m_Sampler), "Failed vkCreateSampler"))
		{
			m_Sampler = VK_NULL_HANDLE;

			Shutdown();

			return;
		}

		m_DescriptorSet = descriptorAllocator.Allocate();

		if (m_DescriptorSet == VK_NULL_HANDLE)
		{
			IG_CORE_ERROR("Vulkan texture initialization aborted: no descriptor set");

			Shutdown();

			return;
		}

		VkDescriptorImageInfo imageInfo{};
		imageInfo.sampler = m_Sampler;
		imageInfo.imageView = m_Image.GetImageView();
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.pImageInfo = &imageInfo;

		vkUpdateDescriptorSets(m_Device, 1, &descriptorWrite, 0, nullptr);

		IG_CORE_INFO("------- VULKAN TEXTURE INITIALIZED -------");
	}

	void VulkanTexture::InitializeFromFile(VkDevice device, VkQueue graphicsQueue, uint32_t graphicsQueueFamily, VmaAllocator allocator, VulkanDescriptorAllocator& descriptorAllocator, const std::string& filepath)
	{
		stbi_set_flip_vertically_on_load(1);

		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_uc* pixels = stbi_load(filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

		if (!pixels)
		{
			IG_CORE_ERROR("Failed to load texture '{}': {}", filepath, stbi_failure_reason());

			return;
		}

		Initialize(device, graphicsQueue, graphicsQueueFamily, allocator, descriptorAllocator, pixels, static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		stbi_image_free(pixels);
	}

	void VulkanTexture::Shutdown()
	{
		if (m_DescriptorSet != VK_NULL_HANDLE && m_DescriptorAllocator)
		{
			m_DescriptorAllocator->Free(m_DescriptorSet);
			m_DescriptorSet = VK_NULL_HANDLE;
		}

		if (m_Sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(m_Device, m_Sampler, nullptr);
			m_Sampler = VK_NULL_HANDLE;
		}

		m_Image.Shutdown();

		m_Device = VK_NULL_HANDLE;
		m_DescriptorAllocator = nullptr;
	}
}