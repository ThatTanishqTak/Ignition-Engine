#include "Ignition/UI/UI.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <ImGuizmo.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>

#include <array>

namespace
{
	ImGuizmo::OPERATION ToImGuizmoOperation(Ignition::UI::GizmoOperation operation)
	{
		switch (operation)
		{
			case Ignition::UI::GizmoOperation::Translate: return ImGuizmo::TRANSLATE;
			case Ignition::UI::GizmoOperation::Rotate: return ImGuizmo::ROTATE;
			case Ignition::UI::GizmoOperation::Scale: return ImGuizmo::SCALE;
		}

		return ImGuizmo::TRANSLATE;
	}

	bool CanDraw()
	{
		const ImGuiContext* context = ImGui::GetCurrentContext();

		return context && context->WithinFrameScope;
	}

	ImGuiCond ToImGuiCond(Ignition::UI::Condition condition)
	{
		switch (condition)
		{
			case Ignition::UI::Condition::Always: return ImGuiCond_Always;
			case Ignition::UI::Condition::Once: return ImGuiCond_Once;
			case Ignition::UI::Condition::FirstUseEver: return ImGuiCond_FirstUseEver;
			case Ignition::UI::Condition::Appearing: return ImGuiCond_Appearing;
		}

		return ImGuiCond_FirstUseEver;
	}
}

namespace Ignition
{
	namespace UI
	{
		bool IsFrameActive()
		{
			return CanDraw();
		}

		void* GetContext()
		{
			return ImGui::GetCurrentContext();
		}

		void GetAllocatorFunctions(void*& allocateFunction, void*& freeFunction, void*& userData)
		{
			ImGuiMemAllocFunc allocate = nullptr;
			ImGuiMemFreeFunc release = nullptr;
			void* user = nullptr;

			ImGui::GetAllocatorFunctions(&allocate, &release, &user);

			allocateFunction = reinterpret_cast<void*>(allocate);
			freeFunction = reinterpret_cast<void*>(release);
			userData = user;
		}

		void SetNextWindowPosition(float x, float y, Condition condition)
		{
			if (CanDraw())
			{
				ImGui::SetNextWindowPos(ImVec2(x, y), ToImGuiCond(condition));
			}
		}

		void SetNextWindowSize(float width, float height, Condition condition)
		{
			if (CanDraw())
			{
				ImGui::SetNextWindowSize(ImVec2(width, height), ToImGuiCond(condition));
			}
		}

		bool BeginWindow(const char* title, bool* open)
		{
			return CanDraw() ? ImGui::Begin(title, open) : false;
		}

		void EndWindow()
		{
			if (CanDraw())
			{
				ImGui::End();
			}
		}

		bool IsWindowHovered()
		{
			return CanDraw() ? ImGui::IsWindowHovered() : false;
		}

		bool IsWindowFocused()
		{
			return CanDraw() ? ImGui::IsWindowFocused() : false;
		}

		glm::vec2 GetCursorScreenPosition()
		{
			if (!CanDraw())
			{
				return glm::vec2(0.0f);
			}

			const ImVec2 position = ImGui::GetCursorScreenPos();

			return glm::vec2(position.x, position.y);
		}

		glm::vec2 GetContentRegionAvailable()
		{
			if (!CanDraw())
			{
				return glm::vec2(0.0f);
			}

			const ImVec2 size = ImGui::GetContentRegionAvail();

			return glm::vec2(size.x, size.y);
		}

		bool WantsTextInput()
		{
			return ImGui::GetCurrentContext() && ImGui::GetIO().WantTextInput;
		}

		unsigned int DockSpaceOverMainViewport()
		{
			if (!CanDraw())
			{
				return 0;
			}

			static int lastSubmittedFrame = -1;
			static unsigned int dockspaceID = 0;

			if (lastSubmittedFrame != ImGui::GetFrameCount())
			{
				dockspaceID = ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
				lastSubmittedFrame = ImGui::GetFrameCount();
			}

			return dockspaceID;
		}

		bool BuildDefaultDockLayout(unsigned int dockspaceID, const char* leftPanel, const char* rightPanel, const char* bottomPanel, const char* centerPanel)
		{
			if (!CanDraw() || dockspaceID == 0)
			{
				return false;
			}

			// A node with children means the user already has a saved layout; leave it alone
			const ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspaceID);

			if (!node || !node->IsLeafNode())
			{
				return false;
			}

			ImGui::DockBuilderRemoveNode(dockspaceID);
			ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace);
			ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);

			ImGuiID centerID = dockspaceID;
			const ImGuiID leftID = ImGui::DockBuilderSplitNode(centerID, ImGuiDir_Left, 0.18f, nullptr, &centerID);
			const ImGuiID rightID = ImGui::DockBuilderSplitNode(centerID, ImGuiDir_Right, 0.22f, nullptr, &centerID);
			const ImGuiID bottomID = ImGui::DockBuilderSplitNode(centerID, ImGuiDir_Down, 0.25f, nullptr, &centerID);

			ImGui::DockBuilderDockWindow(leftPanel, leftID);
			ImGui::DockBuilderDockWindow(rightPanel, rightID);
			ImGui::DockBuilderDockWindow(bottomPanel, bottomID);
			ImGui::DockBuilderDockWindow(centerPanel, centerID);
			ImGui::DockBuilderFinish(dockspaceID);

			return true;
		}

		bool BeginMainMenuBar()
		{
			return CanDraw() ? ImGui::BeginMainMenuBar() : false;
		}

		void EndMainMenuBar()
		{
			if (CanDraw())
			{
				ImGui::EndMainMenuBar();
			}
		}

		bool BeginMenu(const char* label)
		{
			return CanDraw() ? ImGui::BeginMenu(label) : false;
		}

		void EndMenu()
		{
			if (CanDraw())
			{
				ImGui::EndMenu();
			}
		}

		bool MenuItem(const char* label, const char* shortcut)
		{
			return CanDraw() ? ImGui::MenuItem(label, shortcut) : false;
		}

		void OpenPopup(const char* id)
		{
			if (CanDraw())
			{
				ImGui::OpenPopup(id);
			}
		}

		bool BeginPopup(const char* id)
		{
			return CanDraw() ? ImGui::BeginPopup(id) : false;
		}

		bool BeginPopupContextItem(const char* id)
		{
			return CanDraw() ? ImGui::BeginPopupContextItem(id) : false;
		}

		bool BeginPopupContextWindow(const char* id)
		{
			return CanDraw() ? ImGui::BeginPopupContextWindow(id, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems) : false;
		}

		void CloseCurrentPopup()
		{
			if (CanDraw())
			{
				ImGui::CloseCurrentPopup();
			}
		}

		void EndPopup()
		{
			if (CanDraw())
			{
				ImGui::EndPopup();
			}
		}

		void PushWindowPadding(float x, float y)
		{
			if (CanDraw())
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(x, y));
			}
		}

		void PopStyleVariable(int count)
		{
			if (CanDraw())
			{
				ImGui::PopStyleVar(count);
			}
		}

		bool CollapsingHeader(const char* label, bool defaultOpen)
		{
			return CanDraw() ? ImGui::CollapsingHeader(label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None) : false;
		}

		void Text(const char* text)
		{
			if (CanDraw())
			{
				ImGui::TextUnformatted(text);
			}
		}

		void TextDisabled(const char* text)
		{
			if (CanDraw())
			{
				ImGui::TextDisabled("%s", text);
			}
		}

		void LabelText(const char* label, const char* text)
		{
			if (CanDraw())
			{
				ImGui::LabelText(label, "%s", text);
			}
		}

		void BulletText(const char* text)
		{
			if (CanDraw())
			{
				ImGui::BulletText("%s", text);
			}
		}

		bool Button(const char* label)
		{
			return CanDraw() ? ImGui::Button(label) : false;
		}

		bool SmallButton(const char* label)
		{
			return CanDraw() ? ImGui::SmallButton(label) : false;
		}

		bool Selectable(const char* label, bool selected)
		{
			return CanDraw() ? ImGui::Selectable(label, selected) : false;
		}

		bool Checkbox(const char* label, bool* value)
		{
			return CanDraw() ? ImGui::Checkbox(label, value) : false;
		}

		bool InputText(const char* label, char* buffer, size_t bufferSize)
		{
			return CanDraw() ? ImGui::InputText(label, buffer, bufferSize) : false;
		}

		bool SliderFloat(const char* label, float* value, float minimum, float maximum)
		{
			return CanDraw() ? ImGui::SliderFloat(label, value, minimum, maximum) : false;
		}

		bool SliderInt(const char* label, int* value, int minimum, int maximum)
		{
			return CanDraw() ? ImGui::SliderInt(label, value, minimum, maximum) : false;
		}

		bool DragFloat(const char* label, float* value, float speed, float minimum, float maximum)
		{
			return CanDraw() ? ImGui::DragFloat(label, value, speed, minimum, maximum) : false;
		}

		bool DragFloat3(const char* label, glm::vec3& value, float speed)
		{
			return CanDraw() ? ImGui::DragFloat3(label, &value.x, speed) : false;
		}

		bool ColorEdit4(const char* label, glm::vec4& color, bool showInputs)
		{
			return CanDraw() ? ImGui::ColorEdit4(label, &color.x, showInputs ? ImGuiColorEditFlags_None : ImGuiColorEditFlags_NoInputs) : false;
		}

		bool Combo(const char* label, int* currentIndex, const char* const items[], int itemCount)
		{
			return CanDraw() ? ImGui::Combo(label, currentIndex, items, itemCount) : false;
		}

		void Image(uint64_t textureID, float width, float height)
		{
			if (CanDraw() && textureID != 0)
			{
				ImGui::Image((ImTextureID)textureID, ImVec2(width, height));
			}
		}

		void ProgressBar(float fraction, const char* overlay)
		{
			if (CanDraw())
			{
				ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0.0f), overlay);
			}
		}

		void PlotLines(const char* label, const float* values, int count, float scaleMinimum, float scaleMaximum, float height, const char* overlay)
		{
			if (CanDraw())
			{
				ImGui::PlotLines(label, values, count, 0, overlay, scaleMinimum, scaleMaximum, ImVec2(0.0f, height));
			}
		}

		void Separator()
		{
			if (CanDraw())
			{
				ImGui::Separator();
			}
		}

		void SeparatorText(const char* text)
		{
			if (CanDraw())
			{
				ImGui::SeparatorText(text);
			}
		}

		void SameLine(float offsetX, float spacing)
		{
			if (CanDraw())
			{
				ImGui::SameLine(offsetX, spacing);
			}
		}

		void Spacing()
		{
			if (CanDraw())
			{
				ImGui::Spacing();
			}
		}

		void PushID(int id)
		{
			if (CanDraw())
			{
				ImGui::PushID(id);
			}
		}

		void PushID(const char* id)
		{
			if (CanDraw())
			{
				ImGui::PushID(id);
			}
		}

		void PopID()
		{
			if (CanDraw())
			{
				ImGui::PopID();
			}
		}

		void PushWindowBorder(float size, const glm::vec4& color)
		{
			if (CanDraw())
			{
				ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, size);
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(color.r, color.g, color.b, color.a));
			}
		}

		void PopWindowBorder()
		{
			if (CanDraw())
			{
				ImGui::PopStyleColor();
				ImGui::PopStyleVar();
			}
		}

		void BeginDisabled(bool disabled)
		{
			if (CanDraw())
			{
				ImGui::BeginDisabled(disabled);
			}
		}

		void EndDisabled()
		{
			if (CanDraw())
			{
				ImGui::EndDisabled();
			}
		}

		void BeginGizmoFrame()
		{
			if (CanDraw())
			{
				ImGuizmo::BeginFrame();
			}
		}

		void SetGizmoViewportRect(const glm::vec2& position, const glm::vec2& size)
		{
			if (!CanDraw())
			{
				return;
			}

			ImGuizmo::SetOrthographic(false);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(position.x, position.y, size.x, size.y);
		}

		bool TransformGizmo(const glm::mat4& view, const glm::mat4& projection, GizmoOperation operation, GizmoMode mode, glm::vec3& position, glm::vec3& rotationRadians, glm::vec3& scale, float snap)
		{
			if (!CanDraw())
			{
				return false;
			}

			std::array<float, 3> translation = { position.x, position.y, position.z };

			const glm::vec3 degrees = glm::degrees(rotationRadians);
			std::array<float, 3> rotationDegrees = { degrees.x, degrees.y, degrees.z };
			std::array<float, 3> scaleValues = { scale.x, scale.y, scale.z };

			glm::mat4 matrix(1.0f);
			ImGuizmo::RecomposeMatrixFromComponents(translation.data(), rotationDegrees.data(), scaleValues.data(), glm::value_ptr(matrix));

			// ImGuizmo only ever scales in local space
			const ImGuizmo::MODE gizmoMode = (mode == GizmoMode::World && operation != GizmoOperation::Scale) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;
			const std::array<float, 3> snapValues = { snap, snap, snap };

			ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection), ToImGuizmoOperation(operation), gizmoMode, glm::value_ptr(matrix), nullptr, snap > 0.0f ? snapValues.data() : nullptr);

			if (!ImGuizmo::IsUsing())
			{
				return false;
			}

			ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(matrix), translation.data(), rotationDegrees.data(), scaleValues.data());

			position = { translation[0], translation[1], translation[2] };
			rotationRadians = glm::radians(glm::vec3(rotationDegrees[0], rotationDegrees[1], rotationDegrees[2]));
			scale = { scaleValues[0], scaleValues[1], scaleValues[2] };

			return true;
		}

		bool IsGizmoInUse()
		{
			return CanDraw() ? ImGuizmo::IsUsing() : false;
		}

		bool IsGizmoHovered()
		{
			return CanDraw() ? ImGuizmo::IsOver() : false;
		}
	}
}