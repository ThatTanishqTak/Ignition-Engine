#pragma once

#include "Ignition/Events/Event.h"

namespace Ignition
{
	class IGNITION_API WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowClose)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class IGNITION_API WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(int width, int height) : m_Width(width), m_Height(height) {}

		int GetWidth() const { return m_Width; }
		int GetHeight() const { return m_Height; }

		IGNITION_EVENT_CLASS_TYPE(WindowResize)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		int m_Width = 0;
		int m_Height = 0;
	};

	class IGNITION_API WindowFocusEvent : public Event
	{
	public:
		WindowFocusEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowFocus)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class IGNITION_API WindowLostFocusEvent : public Event
	{
	public:
		WindowLostFocusEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowLostFocus)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class IGNITION_API WindowMovedEvent : public Event
	{
	public:
		WindowMovedEvent(int x, int y) : m_X(x), m_Y(y) {}

		int GetX() const { return m_X; }
		int GetY() const { return m_Y; }

		IGNITION_EVENT_CLASS_TYPE(WindowMoved)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		int m_X = 0;
		int m_Y = 0;
	};

	class IGNITION_API WindowMinimizedEvent : public Event
	{
	public:
		WindowMinimizedEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowMinimized)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class IGNITION_API WindowRestoredEvent : public Event
	{
	public:
		WindowRestoredEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowRestored)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};
}