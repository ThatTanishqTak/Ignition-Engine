#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Events/GamepadCodes.h"
#include "Ignition/Events/KeyCodes.h"
#include "Ignition/Events/MouseCodes.h"
#include "Ignition/Events/ScanCodes.h"

#include <cstdint>

namespace Ignition
{
	enum class EventType
	{
		None = 0,

		WindowClose,
		WindowResize,
		WindowPixelSizeChanged,
		WindowDisplayScaleChanged,
		WindowDisplayChanged,
		WindowMoved,
		WindowFocus,
		WindowLostFocus,
		WindowMinimized,
		WindowMaximized,
		WindowRestored,
		WindowShown,
		WindowHidden,
		WindowExposed,
		WindowOccluded,
		WindowMouseEnter,
		WindowMouseLeave,
		WindowEnterFullscreen,
		WindowLeaveFullscreen,

		FileDropped,
		TextDropped,

		KeyPressed,
		KeyReleased,
		TextInput,

		MouseButtonPressed,
		MouseButtonReleased,
		MouseMoved,
		MouseScrolled,

		GamepadConnected,
		GamepadDisconnected,
		GamepadRemapped,
		GamepadButtonPressed,
		GamepadButtonReleased,
		GamepadAxisMoved
	};

	enum EventCategory
	{
		EventCategoryNone = 0,
		EventCategoryApplication = 1 << 0,
		EventCategoryInput = 1 << 1,
		EventCategoryKeyboard = 1 << 2,
		EventCategoryMouse = 1 << 3,
		EventCategoryMouseButton = 1 << 4,
		EventCategoryGamepad = 1 << 5
	};

	class IGNITION_API Event
	{
	public:
		virtual ~Event() = default;

		virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		virtual int GetCategoryFlags() const = 0;

		bool IsInCategory(EventCategory category) const { return (GetCategoryFlags() & category) != 0; }

		bool Handled = false;
	};

	class EventDispatcher
	{
	public:
		explicit EventDispatcher(Event& event) : m_Event(event) {}

		template<typename T, typename F>
		bool Dispatch(const F& function)
		{
			if (m_Event.GetEventType() != T::GetStaticType())
			{
				return false;
			}

			m_Event.Handled = m_Event.Handled || function(static_cast<T&>(m_Event));

			return true;
		}

	private:
		Event& m_Event;
	};
}

#define IGNITION_EVENT_CLASS_TYPE(type) \
	static ::Ignition::EventType GetStaticType() { return ::Ignition::EventType::type; } \
	::Ignition::EventType GetEventType() const override { return GetStaticType(); } \
	const char* GetName() const override { return #type; }

#define IGNITION_EVENT_CLASS_CATEGORY(category) \
	int GetCategoryFlags() const override { return category; }