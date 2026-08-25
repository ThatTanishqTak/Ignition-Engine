#pragma once

#include <cstddef>

namespace Ignition
{
	enum class CursorShape
	{
		Arrow = 0,
		TextInput,
		Wait,
		Crosshair,
		Hand,
		NotAllowed,
		ResizeEW,
		ResizeNS,
		ResizeNESW,
		ResizeNWSE,
		ResizeAll,

		Count
	};

	constexpr std::size_t CursorShapeCount = static_cast<std::size_t>(CursorShape::Count);
}