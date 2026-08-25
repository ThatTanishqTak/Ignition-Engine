#pragma once

#include <cstdint>

namespace Ignition
{
	// Values mirror SDL_Keymod so translation is a cast - keep them in step if SDL ever renumbers
	enum class KeyModifiers : uint16_t
	{
		NONE = 0x0000,
		LSHIFT = 0x0001,
		RSHIFT = 0x0002,
		LCTRL = 0x0040,
		RCTRL = 0x0080,
		LALT = 0x0100,
		RALT = 0x0200,
		LGUI = 0x0400,
		RGUI = 0x0800,
		NUM = 0x1000,
		CAPS = 0x2000,
		MODE = 0x4000,
		SCROLL = 0x8000,

		SHIFT = 0x0003,
		CTRL = 0x00c0,
		ALT = 0x0300,
		GUI = 0x0c00
	};

	constexpr KeyModifiers operator|(KeyModifiers left, KeyModifiers right)
	{
		return static_cast<KeyModifiers>(static_cast<uint16_t>(left) | static_cast<uint16_t>(right));
	}

	constexpr KeyModifiers operator&(KeyModifiers left, KeyModifiers right)
	{
		return static_cast<KeyModifiers>(static_cast<uint16_t>(left) & static_cast<uint16_t>(right));
	}

	constexpr bool HasModifier(KeyModifiers modifiers, KeyModifiers test)
	{
		return (static_cast<uint16_t>(modifiers) & static_cast<uint16_t>(test)) != 0;
	}
}