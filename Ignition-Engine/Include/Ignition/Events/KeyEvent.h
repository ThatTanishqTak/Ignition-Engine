#pragma once

#include "Ignition/Events/Event.h"

#include <string>
#include <utility>

namespace Ignition
{
	class TextInputEvent : public Event
	{
	public:
		explicit TextInputEvent(std::string text) : m_Text(std::move(text)) {}

		const std::string& GetText() const { return m_Text; }

		IGNITION_EVENT_CLASS_TYPE(TextInput)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	private:
		std::string m_Text;
	};

	// IME preedit - the composition string in flight before it commits as a TextInputEvent
	class TextEditingEvent : public Event
	{
	public:
		TextEditingEvent(std::string text, int selectionStart, int selectionLength) : m_Text(std::move(text)), m_SelectionStart(selectionStart), m_SelectionLength(selectionLength) {}

		const std::string& GetText() const { return m_Text; }
		int GetSelectionStart() const { return m_SelectionStart; }
		int GetSelectionLength() const { return m_SelectionLength; }

		IGNITION_EVENT_CLASS_TYPE(TextEditing)
			IGNITION_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	private:
		std::string m_Text;
		int m_SelectionStart = 0;
		int m_SelectionLength = 0;
	};

	class KeyEvent : public Event
	{
	public:
		KeyCode GetKeyCode() const { return m_KeyCode; }
		ScanCode GetScanCode() const { return m_ScanCode; }

		// Modifier state as of this keypress, not as of the pump - polling Input after the queue drains loses Ctrl-click
		KeyModifiers GetModifiers() const { return m_Modifiers; }

		IGNITION_EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

	protected:
		KeyEvent(KeyCode keyCode, ScanCode scanCode, KeyModifiers modifiers) : m_KeyCode(keyCode), m_ScanCode(scanCode), m_Modifiers(modifiers) {}

		KeyCode m_KeyCode = KeyCode::UNKNOWN;
		ScanCode m_ScanCode = ScanCode::UNKNOWN;
		KeyModifiers m_Modifiers = KeyModifiers::NONE;
	};

	class KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(KeyCode keyCode, ScanCode scanCode, KeyModifiers modifiers, bool isRepeat) : KeyEvent(keyCode, scanCode, modifiers), m_IsRepeat(isRepeat) {}

		bool IsRepeat() const { return m_IsRepeat; }

		IGNITION_EVENT_CLASS_TYPE(KeyPressed)

	private:
		bool m_IsRepeat = false;
	};

	class KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(KeyCode keyCode, ScanCode scanCode, KeyModifiers modifiers) : KeyEvent(keyCode, scanCode, modifiers) {}

		IGNITION_EVENT_CLASS_TYPE(KeyReleased)
	};
}