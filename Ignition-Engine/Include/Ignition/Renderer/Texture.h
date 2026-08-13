#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	class VulkanTexture;
	class VulkanRenderer;

	class Texture
	{
	public:
		IGNITION_API ~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		VulkanTexture* GetVulkanTexture() const { return m_VulkanTexture.get(); }

	private:
		friend class Renderer;

		Texture(std::unique_ptr<VulkanTexture> vulkanTexture, std::weak_ptr<VulkanRenderer*> renderer);

	private:
		std::unique_ptr<VulkanTexture> m_VulkanTexture;
		std::weak_ptr<VulkanRenderer*> m_Renderer;
	};
}