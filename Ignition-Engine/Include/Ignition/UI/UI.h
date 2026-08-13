#pragma once

#include "Ignition/Core/Export.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <format>
#include <utility>

namespace Ignition
{
	namespace UI
	{
		enum class Condition
		{
			Always = 0,
			Once,
			FirstUseEver,
			Appearing
		};

		IGNITION_API bool IsFrameActive();

		IGNITION_API void SetNextWindowPosition(float x, float y, Condition condition = Condition::FirstUseEver);
		IGNITION_API void SetNextWindowSize(float width, float height, Condition condition = Condition::FirstUseEver);

		IGNITION_API bool BeginWindow(const char* title, bool* open = nullptr);
		IGNITION_API void EndWindow();

		IGNITION_API bool CollapsingHeader(const char* label, bool defaultOpen = true);

		IGNITION_API void Text(const char* text);
		IGNITION_API void TextDisabled(const char* text);
		IGNITION_API void LabelText(const char* label, const char* text);
		IGNITION_API void BulletText(const char* text);

		template<typename... Args>
			requires (sizeof...(Args) > 0)
		void Text(std::format_string<Args...> format, Args&&... args)
		{
			Text(std::format(format, std::forward<Args>(args)...).c_str());
		}

		template<typename... Args>
			requires (sizeof...(Args) > 0)
		void TextDisabled(std::format_string<Args...> format, Args&&... args)
		{
			TextDisabled(std::format(format, std::forward<Args>(args)...).c_str());
		}

		IGNITION_API bool Button(const char* label);
		IGNITION_API bool SmallButton(const char* label);
		IGNITION_API bool Checkbox(const char* label, bool* value);
		IGNITION_API bool SliderFloat(const char* label, float* value, float minimum, float maximum);
		IGNITION_API bool SliderInt(const char* label, int* value, int minimum, int maximum);
		IGNITION_API bool DragFloat(const char* label, float* value, float speed = 0.01f, float minimum = 0.0f, float maximum = 0.0f);
		IGNITION_API bool DragFloat3(const char* label, glm::vec3& value, float speed = 0.01f);
		IGNITION_API bool ColorEdit4(const char* label, glm::vec4& color, bool showInputs = false);
		IGNITION_API bool Combo(const char* label, int* currentIndex, const char* const items[], int itemCount);

		IGNITION_API void ProgressBar(float fraction, const char* overlay = nullptr);
		IGNITION_API void PlotLines(const char* label, const float* values, int count, float scaleMinimum, float scaleMaximum, float height = 80.0f, const char* overlay = nullptr);

		IGNITION_API void Separator();
		IGNITION_API void SeparatorText(const char* text);
		IGNITION_API void SameLine(float offsetX = 0.0f, float spacing = -1.0f);
		IGNITION_API void Spacing();

		IGNITION_API void PushID(int id);
		IGNITION_API void PushID(const char* id);
		IGNITION_API void PopID();
	}
}