#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Input/CursorMode.h"
#include "Ignition/Events/GamepadCodes.h"
#include "Ignition/Events/KeyCodes.h"
#include "Ignition/Events/MouseCodes.h"
#include "Ignition/Events/ScanCodes.h"

#include <glm/vec2.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Ignition
{
	struct InputImplementation;

	class Input
	{
	public:
		~Input();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		IGNITION_API bool IsKeyDown(ScanCode scanCode) const;
		IGNITION_API bool IsKeyPressed(ScanCode scanCode) const;
		IGNITION_API bool IsKeyReleased(ScanCode scanCode) const;

		IGNITION_API bool IsKeyDown(KeyCode keyCode) const;
		IGNITION_API bool IsKeyPressed(KeyCode keyCode) const;
		IGNITION_API bool IsKeyReleased(KeyCode keyCode) const;

		IGNITION_API glm::vec2 GetMousePosition() const;
		IGNITION_API glm::vec2 GetMouseDelta() const;
		IGNITION_API glm::vec2 GetMouseWheel() const;

		IGNITION_API bool IsMouseButtonDown(MouseCode button) const;
		IGNITION_API bool IsMouseButtonPressed(MouseCode button) const;
		IGNITION_API bool IsMouseButtonReleased(MouseCode button) const;

		IGNITION_API void SetCursorMode(CursorMode mode);
		IGNITION_API CursorMode GetCursorMode() const;

		IGNITION_API void SetTextInputEnabled(bool enabled);

		IGNITION_API bool IsGamepadConnected(GamepadID gamepadID) const;
		IGNITION_API std::vector<GamepadID> GetConnectedGamepads() const;
		IGNITION_API std::string GetGamepadName(GamepadID gamepadID) const;

		IGNITION_API bool IsGamepadButtonDown(GamepadID gamepadID, GamepadButton button) const;
		IGNITION_API bool IsGamepadButtonPressed(GamepadID gamepadID, GamepadButton button) const;
		IGNITION_API bool IsGamepadButtonReleased(GamepadID gamepadID, GamepadButton button) const;

		IGNITION_API float GetGamepadAxis(GamepadID gamepadID, GamepadAxis axis) const;
		IGNITION_API float GetGamepadAxisRaw(GamepadID gamepadID, GamepadAxis axis) const;
		IGNITION_API glm::vec2 GetGamepadStick(GamepadID gamepadID, GamepadStick stick) const;

		IGNITION_API bool SetGamepadRumble(GamepadID gamepadID, float lowFrequency, float highFrequency, uint32_t durationMilliseconds);

		IGNITION_API float GetStickDeadzone() const;
		IGNITION_API void SetStickDeadzone(float deadzone);
		IGNITION_API float GetTriggerDeadzone() const;
		IGNITION_API void SetTriggerDeadzone(float deadzone);

	private:
		friend class Engine;

		Input();

		std::unique_ptr<InputImplementation> m_Implementation;
	};
}