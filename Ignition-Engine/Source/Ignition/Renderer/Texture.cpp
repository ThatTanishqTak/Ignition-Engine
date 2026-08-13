#include "Ignition/Renderer/Texture.h"

#include "Ignition/Renderer/Vulkan/VulkanTexture.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

namespace Ignition
{
	Texture::Texture(std::unique_ptr<VulkanTexture> vulkanTexture, std::weak_ptr<VulkanRenderer*> renderer) : m_VulkanTexture(std::move(vulkanTexture)), m_Renderer(std::move(renderer))
	{

	}

	Texture::~Texture()
	{
		if (!m_VulkanTexture)
		{
			return;
		}

		const std::shared_ptr<VulkanRenderer*> renderer = m_Renderer.lock();

		if (renderer && *renderer)
		{
			(*renderer)->Retire(std::move(m_VulkanTexture));
		}
		else
		{
			m_VulkanTexture->Shutdown();
		}
	}
}