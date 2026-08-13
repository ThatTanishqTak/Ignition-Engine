#pragma once

#include "Ignition/Events/GamepadCodes.h"
#include "Ignition/Events/KeyCodes.h"
#include "Ignition/Events/MouseCodes.h"
#include "Ignition/Events/ScanCodes.h"
#include "Ignition/Input/CursorMode.h"

#include <glm/vec2.hpp>

#include <array>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

struct SDL_Gamepad;

namespace Ignition
{
	class Event;
	class Window;

	struct InputImplementation
	{
		struct GamepadState
		{
			SDL_Gamepad* Handle = nullptr;
			std::array<bool, static_cast<size_t>(GamepadButton::COUNT)> Down{};
			std::array<bool, static_cast<size_t>(GamepadButton::COUNT)> Pressed{};
			std::array<bool, static_cast<size_t>(GamepadButton::COUNT)> Released{};
			std::array<float, static_cast<size_t>(GamepadAxis::COUNT)> Axes{};
		};

		void Initialize(Window* window);
		void Shutdown();

		void NewFrame();
		void OnEvent(Event& event);

		void OpenGamepad(GamepadID gamepadID);
		void CloseGamepad(GamepadID gamepadID);
		void ClearHeldState();

		const GamepadState* FindGamepad(GamepadID gamepadID) const;
		GamepadState* FindGamepad(GamepadID gamepadID);

		Window* WindowHandle = nullptr;

		std::array<bool, static_cast<size_t>(ScanCode::COUNT)> KeysDown{};
		std::array<bool, static_cast<size_t>(ScanCode::COUNT)> KeysPressed{};
		std::array<bool, static_cast<size_t>(ScanCode::COUNT)> KeysReleased{};

		std::unordered_set<KeyCode> KeyCodesDown;
		std::unordered_set<KeyCode> KeyCodesPressed;
		std::unordered_set<KeyCode> KeyCodesReleased;

		glm::vec2 MousePosition{ 0.0f, 0.0f };
		glm::vec2 MouseDelta{ 0.0f, 0.0f };
		glm::vec2 MouseWheel{ 0.0f, 0.0f };

		std::array<bool, 8> MouseDown{};
		std::array<bool, 8> MousePressed{};
		std::array<bool, 8> MouseReleased{};

		std::unordered_map<GamepadID, GamepadState> Gamepads;

		float StickDeadzone = 0.15f;
		float TriggerDeadzone = 0.05f;

		CursorMode Cursor = CursorMode::Normal;
	};
}