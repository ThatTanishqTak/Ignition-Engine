#pragma once

#include <cstdint>

namespace Ignition
{
	enum class MouseCode : uint8_t
	{
		UNKNOWN = 0,
		LEFT = 1,
		MIDDLE = 2,
		RIGHT = 3,
		X1 = 4,
		X2 = 5
	};
}