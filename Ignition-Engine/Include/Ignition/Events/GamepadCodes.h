#pragma once

#include <cstdint>

namespace Ignition
{
	enum class GamepadButton : int32_t
	{
		INVALID = -1,
		SOUTH = 0,
		EAST = 1,
		WEST = 2,
		NORTH = 3,
		BACK = 4,
		GUIDE = 5,
		START = 6,
		LEFT_STICK = 7,
		RIGHT_STICK = 8,
		LEFT_SHOULDER = 9,
		RIGHT_SHOULDER = 10,
		DPAD_UP = 11,
		DPAD_DOWN = 12,
		DPAD_LEFT = 13,
		DPAD_RIGHT = 14,
		MISC1 = 15,
		RIGHT_PADDLE1 = 16,
		LEFT_PADDLE1 = 17,
		RIGHT_PADDLE2 = 18,
		LEFT_PADDLE2 = 19,
		TOUCHPAD = 20,
		MISC2 = 21,
		MISC3 = 22,
		MISC4 = 23,
		MISC5 = 24,
		MISC6 = 25,
		COUNT = 26
	};

	enum class GamepadAxis : int32_t
	{
		INVALID = -1,
		LEFTX = 0,
		LEFTY = 1,
		RIGHTX = 2,
		RIGHTY = 3,
		LEFT_TRIGGER = 4,
		RIGHT_TRIGGER = 5,
		COUNT = 6
	};
}