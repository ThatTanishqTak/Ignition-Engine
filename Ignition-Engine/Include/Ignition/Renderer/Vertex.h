#pragma once

#include <glm/vec3.hpp>
#include <glm/vec2.hpp>

namespace Ignition
{
	struct Vertex
	{
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec3 Color;
		glm::vec2 UV;
	};
}