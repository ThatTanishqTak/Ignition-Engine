#pragma once

#include "Ignition/Events/Event.h"

namespace Ignition
{
	class MouseMovedEvent : public Event
	{
	public:
		MouseMovedEvent(float x, float y, float deltaX, float deltaY) : m_X(x), m_Y(y), m_DeltaX(deltaX), m_DeltaY(deltaY) {}

		float GetX() const { return m_X; }
		float GetY() const { return m_Y; }

		float GetDeltaX() const { return m_DeltaX; }
		float GetDeltaY() const { return m_DeltaY; }

		IGNITION_EVENT_CLASS_TYPE(MouseMoved)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_X = 0.0f;
		float m_Y = 0.0f;
		float m_DeltaX = 0.0f;
		float m_DeltaY = 0.0f;
	};

	class MouseScrolledEvent : public Event
	{
	public:
		MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

		float GetXOffset() const { return m_XOffset; }
		float GetYOffset() const { return m_YOffset; }

		IGNITION_EVENT_CLASS_TYPE(MouseScrolled)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)

	private:
		float m_XOffset = 0.0f;
		float m_YOffset = 0.0f;
	};

	class MouseButtonEvent : public Event
	{
	public:
		MouseCode GetMouseButton() const { return m_Button; }

		// Modifier state as of this click - what Ctrl-click, Shift-click and Alt-drag are routed on
		KeyModifiers GetModifiers() const { return m_Modifiers; }

		IGNITION_EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryMouseButton | EventCategoryInput)

	protected:
		MouseButtonEvent(MouseCode button, KeyModifiers modifiers) : m_Button(button), m_Modifiers(modifiers) {}

		MouseCode m_Button = MouseCode::UNKNOWN;
		KeyModifiers m_Modifiers = KeyModifiers::NONE;
	};

	class MouseButtonPressedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonPressedEvent(MouseCode button, KeyModifiers modifiers) : MouseButtonEvent(button, modifiers) {}

		IGNITION_EVENT_CLASS_TYPE(MouseButtonPressed)
	};

	class MouseButtonReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseButtonReleasedEvent(MouseCode button, KeyModifiers modifiers) : MouseButtonEvent(button, modifiers) {}

		IGNITION_EVENT_CLASS_TYPE(MouseButtonReleased)
	};
}