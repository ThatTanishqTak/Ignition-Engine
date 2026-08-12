#pragma once

#include "Ignition/Core/Export.h"

struct ImGuiContext;

namespace Ignition
{
	namespace UI
	{
		IGNITION_API ImGuiContext* GetImGuiContext();
		IGNITION_API void GetImGuiAllocatorFunctions(void* (**allocFunc)(size_t size, void* userData), void (**freeFunc)(void* pointer, void* userData), void** userData);
	}
}