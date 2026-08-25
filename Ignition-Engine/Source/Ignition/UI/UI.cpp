#include "Ignition/UI/UI.h"

namespace Ignition
{
	namespace UI
	{
		bool IsFrameActive() { return false; }

		void* GetContext() { return nullptr; }

		void GetAllocatorFunctions(void*& allocateFunction, void*& freeFunction, void*& userData)
		{
			allocateFunction = nullptr;
			freeFunction = nullptr;
			userData = nullptr;
		}

		void SetNextWindowPosition(float, float, Condition) {}
		void SetNextWindowSize(float, float, Condition) {}

		bool BeginWindow(const char*, bool*) { return false; }
		void EndWindow() {}

		bool IsWindowHovered() { return false; }
		bool IsWindowFocused() { return false; }
		glm::vec2 GetCursorScreenPosition() { return glm::vec2(0.0f); }
		glm::vec2 GetContentRegionAvailable() { return glm::vec2(0.0f); }
		bool WantsTextInput() { return false; }

		unsigned int DockSpaceOverMainViewport() { return 0; }
		bool BuildDefaultDockLayout(unsigned int, const char*, const char*, const char*, const char*) { return false; }

		bool BeginMainMenuBar() { return false; }
		void EndMainMenuBar() {}
		bool BeginMenu(const char*) { return false; }
		void EndMenu() {}
		bool MenuItem(const char*, const char*) { return false; }

		void OpenPopup(const char*) {}
		bool BeginPopup(const char*) { return false; }
		bool BeginPopupContextItem(const char*) { return false; }
		bool BeginPopupContextWindow(const char*) { return false; }
		void CloseCurrentPopup() {}
		void EndPopup() {}

		void PushWindowPadding(float, float) {}
		void PopStyleVariable(int) {}

		void PushWindowBorder(float, const glm::vec4&) {}
		void PopWindowBorder() {}

		void BeginDisabled(bool) {}
		void EndDisabled() {}

		bool CollapsingHeader(const char*, bool) { return false; }

		bool TreeNode(const char*, bool, bool, bool) { return false; }
		void TreePop() {}
		bool IsItemClicked() { return false; }

		void Text(const char*) {}
		void TextDisabled(const char*) {}
		void LabelText(const char*, const char*) {}
		void BulletText(const char*) {}

		bool Button(const char*) { return false; }
		bool SmallButton(const char*) { return false; }
		bool Selectable(const char*, bool) { return false; }
		bool Checkbox(const char*, bool*) { return false; }
		bool InputText(const char*, char*, size_t, bool) { return false; }
		bool SliderFloat(const char*, float*, float, float) { return false; }
		bool SliderInt(const char*, int*, int, int) { return false; }
		bool DragFloat(const char*, float*, float, float, float) { return false; }
		bool DragFloat3(const char*, glm::vec3&, float) { return false; }
		bool ColorEdit4(const char*, glm::vec4&, bool) { return false; }
		bool Combo(const char*, int*, const char* const [], int) { return false; }

		void Image(uint64_t, float, float) {}

		void ProgressBar(float, const char*) {}
		void PlotLines(const char*, const float*, int, float, float, float, const char*) {}

		void Separator() {}
		void SeparatorText(const char*) {}
		void SameLine(float, float) {}
		void Spacing() {}

		void PushID(int) {}
		void PushID(const char*) {}
		void PopID() {}

		void BeginGizmoFrame() {}
		void SetGizmoViewportRect(const glm::vec2&, const glm::vec2&) {}
		bool TransformGizmo(const glm::mat4&, const glm::mat4&, GizmoOperation, GizmoMode, glm::vec3&, glm::quat&, glm::vec3&, float) { return false; }

		bool IsGizmoInUse() { return false; }
		bool IsGizmoHovered() { return false; }
	}
}