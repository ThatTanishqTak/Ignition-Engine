#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace Ignition
{
	struct PushConstantData
	{
		glm::mat4 MVP;
		glm::vec4 Tint;
	};

	static_assert(sizeof(PushConstantData) == 80, "Push constant block must stay 80 bytes to match the layout expected by the shaders");
}