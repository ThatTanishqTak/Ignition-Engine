#pragma once

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace Ignition
{
	namespace UI
	{
		struct Style
		{
			glm::vec4 BackgroundColor{ 0.0f, 0.0f, 0.0f, 0.0f };
			glm::vec4 BorderColor{ 0.0f, 0.0f, 0.0f, 0.0f };
			glm::vec4 ForegroundColor{ 1.0f, 1.0f, 1.0f, 1.0f };

			float BorderWidth = 0.0f;
			float CornerRadius = 0.0f;

			glm::vec2 Padding{ 0.0f };
			glm::vec2 Margin{ 0.0f };
		};
	}
}