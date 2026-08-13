#pragma once

#include <glm/vec4.hpp>

#include <memory>

namespace Ignition
{
	class Texture;

	struct Material
	{
		std::shared_ptr<Texture> Albedo;
		glm::vec4 Tint{ 1.0f };
		bool TwoSided = false;
	};
}