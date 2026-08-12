#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	class VulkanTexture;

	class Texture
	{
	public:
		IGNITION_API ~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		VulkanTexture* GetVulkanTexture() const { return m_VulkanTexture.get(); }

	private:
		friend class Renderer;

		explicit Texture(std::unique_ptr<VulkanTexture> vulkanTexture);

	private:
		std::unique_ptr<VulkanTexture> m_VulkanTexture;
	};
}