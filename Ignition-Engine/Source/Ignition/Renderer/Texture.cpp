#include "Ignition/Renderer/Texture.h"

#include "Ignition/Renderer/TextureImplementation.h"

namespace Ignition
{
	Texture::Texture() : m_Implementation(std::make_unique<TextureImplementation>())
	{

	}

	Texture::~Texture()
	{
		if (!m_Implementation->Handle)
		{
			return;
		}

		const std::shared_ptr<VulkanRenderer*> renderer = m_Implementation->Backend.lock();

		if (renderer && *renderer)
		{
			(*renderer)->Retire(std::move(m_Implementation->Handle));
		}
		else
		{
			m_Implementation->Handle->Shutdown();
		}
	}
}