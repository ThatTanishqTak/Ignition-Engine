#include "Ignition/Renderer/Texture.h"

#include "Ignition/Renderer/TextureImplementation.h"

namespace Ignition
{
	Texture::Texture() : m_Implementation(std::make_unique<TextureImplementation>())
	{

	}

	Texture::~Texture() = default;
}