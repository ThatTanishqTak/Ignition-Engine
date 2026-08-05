#pragma once

#include "Ignition/Events/Event.h"

#include <string>
#include <utility>

namespace Ignition
{
	class IGNITION_API TextInputEvent : public Event
	{
	public:
		explicit TextInputEvent(std::string text) : m_Text(std::move(text)) {}

		const std::string& GetText() const { return m_Text; }

		IGNITION_EVENT_CLASS_TYPE(TextInput)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	private:
		std::string m_Text;
	};

	class IGNITION_API KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }
		ScanCode GetScanCode() const { return m_ScanCode; }

		IGNITION_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(KeyCode keyCode, ScanCode scanCode) : m_KeyCode(keyCode), m_ScanCode(scanCode) {}

		KeyCode m_KeyCode = KeyCode::UNKNOWN;
		ScanCode m_ScanCode = ScanCode::UNKNOWN;
	};

	class IGNITION_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode keyCode, ScanCode scanCode, bool isRepeat) : KeyEvent(keyCode, scanCode), m_IsRepeat(isRepeat) {}

		bool IsRepeat() const { return m_IsRepeat; }

		IGNITION_EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat = false;
	};

	class IGNITION_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keyCode, ScanCode scanCode) : KeyEvent(keyCode, scanCode) {}

		IGNITION_EVENT_CLASS_TYPE(KeyReleased)
	};
}