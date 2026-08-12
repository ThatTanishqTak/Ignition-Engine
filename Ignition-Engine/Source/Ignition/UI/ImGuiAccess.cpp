#include "Ignition/UI/ImGuiAccess.h"

#include <imgui.h>

namespace Ignition
{
	namespace UI
	{
		ImGuiContext* GetImGuiContext()
		{
			return ImGui::GetCurrentContext();
		}

		void GetImGuiAllocatorFunctions(void* (**allocFunc)(size_t size, void* userData), void (**freeFunc)(void* pointer, void* userData), void** userData)
		{
			ImGuiMemAllocFunc engineAlloc = nullptr;
			ImGuiMemFreeFunc engineFree = nullptr;
			void* engineUserData = nullptr;

			ImGui::GetAllocatorFunctions(&engineAlloc, &engineFree, &engineUserData);

			*allocFunc = engineAlloc;
			*freeFunc = engineFree;
			*userData = engineUserData;
		}
	}
}