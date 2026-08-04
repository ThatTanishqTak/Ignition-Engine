#pragma once

#include "Ignition/Events/Event.h"

namespace Ignition
{
	class IGNITION_API KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }

		IGNITION_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		explicit KeyEvent(KeyCode keyCode) : m_KeyCode(keyCode) {}

		KeyCode m_KeyCode = 0;
	};

	class IGNITION_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode keyCode, bool isRepeat) : KeyEvent(keyCode), m_IsRepeat(isRepeat) {}

		bool IsRepeat() const { return m_IsRepeat; }

		IGNITION_EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat = false;
	};

	class IGNITION_API KeyReleasedEvent : public KeyEvent
	{
	public:
		explicit KeyReleasedEvent(KeyCode keyCode) : KeyEvent(keyCode) {}

		IGNITION_EVENT_CLASS_TYPE(KeyReleased)
	};
}