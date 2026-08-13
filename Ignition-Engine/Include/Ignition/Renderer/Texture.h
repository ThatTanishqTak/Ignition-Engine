#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	struct TextureImplementation;

	class Texture
	{
	public:
		IGNITION_API ~Texture();

		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

	private:
		friend class Renderer;

		Texture();

		std::unique_ptr<TextureImplementation> m_Implementation;
	};
}