#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Input/CursorMode.h"
#include "Ignition/Events/GamepadCodes.h"
#include "Ignition/Events/KeyCodes.h"
#include "Ignition/Events/MouseCodes.h"
#include "Ignition/Events/ScanCodes.h"

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct SDL_Gamepad;

namespace Ignition
{
	class Event;
	class Window;

	// Placeholder math type until glm is wired up.
	struct Float2
	{
		float X = 0.0f;
		float Y = 0.0f;
	};

	class Input
	{
	public:
		Input();
		~Input();

		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		void Initialize(Window* window);
		void Shutdown();

		void NewFrame();
		void OnEvent(Event& event);

		IGNITION_API bool IsKeyDown(ScanCode scanCode) const;
		IGNITION_API bool IsKeyPressed(ScanCode scanCode) const;
		IGNITION_API bool IsKeyReleased(ScanCode scanCode) const;

		IGNITION_API bool IsKeyDown(KeyCode keyCode) const;
		IGNITION_API bool IsKeyPressed(KeyCode keyCode) const;
		IGNITION_API bool IsKeyReleased(KeyCode keyCode) const;

		IGNITION_API Float2 GetMousePosition() const;
		IGNITION_API Float2 GetMouseDelta() const;
		IGNITION_API Float2 GetMouseWheel() const;

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
		IGNITION_API Float2 GetGamepadStick(GamepadID gamepadID, GamepadStick stick) const;

		IGNITION_API bool SetGamepadRumble(GamepadID gamepadID, float lowFrequency, float highFrequency, uint32_t durationMilliseconds);

		IGNITION_API float GetStickDeadzone() const;
		IGNITION_API void SetStickDeadzone(float deadzone);
		IGNITION_API float GetTriggerDeadzone() const;
		IGNITION_API void SetTriggerDeadzone(float deadzone);

	private:
		struct GamepadState
		{
			SDL_Gamepad* Handle = nullptr;
			std::array<bool, static_cast<size_t>(GamepadButton::COUNT)> Down{};
			std::array<bool, static_cast<size_t>(GamepadButton::COUNT)> Pressed{};
			std::array<bool, static_cast<size_t>(GamepadButton::COUNT)> Released{};
			std::array<float, static_cast<size_t>(GamepadAxis::COUNT)> Axes{};
		};

		void OpenGamepad(GamepadID gamepadID);
		void CloseGamepad(GamepadID gamepadID);
		void ClearHeldState();

		const GamepadState* FindGamepad(GamepadID gamepadID) const;
		GamepadState* FindGamepad(GamepadID gamepadID);

	private:
		Window* m_Window = nullptr;

		std::array<bool, static_cast<size_t>(ScanCode::COUNT)> m_KeysDown{};
		std::array<bool, static_cast<size_t>(ScanCode::COUNT)> m_KeysPressed{};
		std::array<bool, static_cast<size_t>(ScanCode::COUNT)> m_KeysReleased{};

		std::unordered_set<KeyCode> m_KeyCodesDown;
		std::unordered_set<KeyCode> m_KeyCodesPressed;
		std::unordered_set<KeyCode> m_KeyCodesReleased;

		Float2 m_MousePosition{};
		Float2 m_MouseDelta{};
		Float2 m_MouseWheel{};

		std::array<bool, 8> m_MouseDown{};
		std::array<bool, 8> m_MousePressed{};
		std::array<bool, 8> m_MouseReleased{};

		std::unordered_map<GamepadID, GamepadState> m_Gamepads;

		float m_StickDeadzone = 0.15f;
		float m_TriggerDeadzone = 0.05f;

		CursorMode m_CursorMode = CursorMode::Normal;
	};
}