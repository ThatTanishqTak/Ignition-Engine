#pragma once

#include "Ignition/Events/Event.h"

namespace Ignition
{
	using GamepadID = uint32_t;

	class GamepadEvent : public Event
	{
	public:
		GamepadID GetGamepadID() const { return m_GamepadID; }

		IGNITION_EVENT_CLASS_CATEGORY(EventCategoryGamepad | EventCategoryInput)

	protected:
		explicit GamepadEvent(GamepadID gamepadID) : m_GamepadID(gamepadID) {}

		GamepadID m_GamepadID = 0;
	};

	class GamepadConnectedEvent : public GamepadEvent
	{
	public:
		explicit GamepadConnectedEvent(GamepadID gamepadID) : GamepadEvent(gamepadID) {}

		IGNITION_EVENT_CLASS_TYPE(GamepadConnected)
	};

	class GamepadDisconnectedEvent : public GamepadEvent
	{
	public:
		explicit GamepadDisconnectedEvent(GamepadID gamepadID) : GamepadEvent(gamepadID) {}

		IGNITION_EVENT_CLASS_TYPE(GamepadDisconnected)
	};

	class GamepadRemappedEvent : public GamepadEvent
	{
	public:
		explicit GamepadRemappedEvent(GamepadID gamepadID) : GamepadEvent(gamepadID) {}

		IGNITION_EVENT_CLASS_TYPE(GamepadRemapped)
	};

	class GamepadButtonEvent : public GamepadEvent
	{
	public:
		GamepadButton GetButton() const { return m_Button; }

	protected:
		GamepadButtonEvent(GamepadID gamepadID, GamepadButton button) : GamepadEvent(gamepadID), m_Button(button) {}

		GamepadButton m_Button = GamepadButton::INVALID;
	};

	class GamepadButtonPressedEvent : public GamepadButtonEvent
	{
	public:
		GamepadButtonPressedEvent(GamepadID gamepadID, GamepadButton button) : GamepadButtonEvent(gamepadID, button) {}

		IGNITION_EVENT_CLASS_TYPE(GamepadButtonPressed)
	};

	class GamepadButtonReleasedEvent : public GamepadButtonEvent
	{
	public:
		GamepadButtonReleasedEvent(GamepadID gamepadID, GamepadButton button) : GamepadButtonEvent(gamepadID, button) {}

		IGNITION_EVENT_CLASS_TYPE(GamepadButtonReleased)
	};

	class GamepadAxisMovedEvent : public GamepadEvent
	{
	public:
		GamepadAxisMovedEvent(GamepadID gamepadID, GamepadAxis axis, float value, int16_t rawValue) : GamepadEvent(gamepadID), m_Axis(axis), m_Value(value), m_RawValue(rawValue)
		{
		}

		GamepadAxis GetAxis() const { return m_Axis; }
		float GetValue() const { return m_Value; }
		int16_t GetRawValue() const { return m_RawValue; }

		IGNITION_EVENT_CLASS_TYPE(GamepadAxisMoved)

	private:
		GamepadAxis m_Axis = GamepadAxis::INVALID;
		float m_Value = 0.0f;
		int16_t m_RawValue = 0;
	};
}