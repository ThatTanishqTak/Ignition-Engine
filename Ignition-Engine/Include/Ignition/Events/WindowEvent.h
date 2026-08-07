#pragma once

#include "Ignition/Events/Event.h"

#include <string>

namespace Ignition
{
	class WindowCloseEvent : public Event
	{
	public:
		WindowCloseEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowClose)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowResizeEvent : public Event
	{
	public:
		WindowResizeEvent(int pixelWidth, int pixelHeight) : m_PixelWidth(pixelWidth), m_PixelHeight(pixelHeight) {}

		int GetPixelWidth() const { return m_PixelWidth; }
		int GetPixelHeight() const { return m_PixelHeight; }

		IGNITION_EVENT_CLASS_TYPE(WindowResize)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		int m_PixelWidth = 0;
		int m_PixelHeight = 0;
	};

	class WindowDisplayScaleChangedEvent : public Event
	{
	public:
		WindowDisplayScaleChangedEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowDisplayScaleChanged)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowDisplayChangedEvent : public Event
	{
	public:
		explicit WindowDisplayChangedEvent(int displayIndex) : m_DisplayIndex(displayIndex) {}

		int GetDisplayIndex() const { return m_DisplayIndex; }

		IGNITION_EVENT_CLASS_TYPE(WindowDisplayChanged)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		int m_DisplayIndex = 0;
	};

	class WindowMovedEvent : public Event
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

	class WindowFocusEvent : public Event
	{
	public:
		WindowFocusEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowFocus)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowLostFocusEvent : public Event
	{
	public:
		WindowLostFocusEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowLostFocus)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowMinimizedEvent : public Event
	{
	public:
		WindowMinimizedEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowMinimized)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowMaximizedEvent : public Event
	{
	public:
		WindowMaximizedEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowMaximized)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowRestoredEvent : public Event
	{
	public:
		WindowRestoredEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowRestored)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowShownEvent : public Event
	{
	public:
		WindowShownEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowShown)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowHiddenEvent : public Event
	{
	public:
		WindowHiddenEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowHidden)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowExposedEvent : public Event
	{
	public:
		WindowExposedEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowExposed)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowOccludedEvent : public Event
	{
	public:
		WindowOccludedEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowOccluded)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowMouseEnterEvent : public Event
	{
	public:
		WindowMouseEnterEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowMouseEnter)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowMouseLeaveEvent : public Event
	{
	public:
		WindowMouseLeaveEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowMouseLeave)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowEnterFullscreenEvent : public Event
	{
	public:
		WindowEnterFullscreenEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowEnterFullscreen)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class WindowLeaveFullscreenEvent : public Event
	{
	public:
		WindowLeaveFullscreenEvent() = default;

		IGNITION_EVENT_CLASS_TYPE(WindowLeaveFullscreen)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)
	};

	class FileDroppedEvent : public Event
	{
	public:
		FileDroppedEvent(std::string path, float x, float y) : m_Path(std::move(path)), m_X(x), m_Y(y) {}

		const std::string& GetPath() const { return m_Path; }
		float GetX() const { return m_X; }
		float GetY() const { return m_Y; }

		IGNITION_EVENT_CLASS_TYPE(FileDropped)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		std::string m_Path;
		float m_X = 0.0f;
		float m_Y = 0.0f;
	};

	class TextDroppedEvent : public Event
	{
	public:
		TextDroppedEvent(std::string text, float x, float y) : m_Text(std::move(text)), m_X(x), m_Y(y) {}

		const std::string& GetText() const { return m_Text; }
		float GetX() const { return m_X; }
		float GetY() const { return m_Y; }

		IGNITION_EVENT_CLASS_TYPE(TextDropped)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryApplication)

	private:
		std::string m_Text;
		float m_X = 0.0f;
		float m_Y = 0.0f;
	};
}