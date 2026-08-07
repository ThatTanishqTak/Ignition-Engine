#pragma once

namespace Ignition
{
	enum class CursorMode
	{
		Normal = 0,   // Visible, moves freely
		Hidden,       // Invisible, moves freely (custom cursors / crosshairs)
		Captured      // Invisible, position frozen, relative deltas keep flowing (FPS camera)
	};
}