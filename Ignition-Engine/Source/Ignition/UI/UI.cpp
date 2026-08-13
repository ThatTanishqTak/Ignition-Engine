#include "Ignition/UI/UI.h"

#include <imgui.h>
#include <imgui_internal.h>

namespace
{
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

		bool Checkbox(const char* label, bool* value)
		{
			return CanDraw() ? ImGui::Checkbox(label, value) : false;
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
	}
}