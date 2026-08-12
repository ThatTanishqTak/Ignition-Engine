#include "Ignition/Renderer/Texture.h"

#include "Ignition/Renderer/Vulkan/VulkanTexture.h"

namespace Ignition
{
	Texture::Texture(std::unique_ptr<VulkanTexture> vulkanTexture) : m_VulkanTexture(std::move(vulkanTexture))
	{

	}

	Texture::~Texture()
	{
		if (m_VulkanTexture)
		{
			m_VulkanTexture->Shutdown();
		}
	}
}