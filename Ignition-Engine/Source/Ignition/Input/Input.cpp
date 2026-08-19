#include "Ignition/Input/Input.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/GamepadEvent.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Input/InputImplementation.h"
#include "Ignition/Window/Window.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace
{
	glm::vec2 ApplyRadialDeadzone(float x, float y, float deadzone)
	{
		const float magnitude = std::sqrt(x * x + y * y);

		if (magnitude < deadzone)
		{
			return glm::vec2(0.0f);
		}

		// Remap so output runs continuously from 0 at the deadzone edge to 1 at full deflection
		const float normalized = std::min((magnitude - deadzone) / (1.0f - deadzone), 1.0f);
		const float scale = normalized / magnitude;

		return { x * scale, y * scale };
	}

	float ApplyThresholdDeadzone(float value, float deadzone)
	{
		if (value < deadzone)
		{
			return 0.0f;
		}

		return std::min((value - deadzone) / (1.0f - deadzone), 1.0f);
	}
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

namespace Ignition
{
	void InputImplementation::Initialize(Window* window)
	{
		IG_CORE_INFO("------- INITIALIZING INPUT -------");

		WindowHandle = window;

		IG_CORE_INFO("------- INPUT INITIALIZED -------");
	}

	void InputImplementation::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN INPUT -------");

		for (auto& [gamepadID, state] : Gamepads)
		{
			if (state.Handle)
			{
				SDL_CloseGamepad(state.Handle);
			}
		}

		Gamepads.clear();
		WindowHandle = nullptr;

		IG_CORE_INFO("------- INPUT SHUTDOWN COMPLETE -------");
	}

	void InputImplementation::NewFrame()
	{
		KeysPressed.fill(false);
		KeysReleased.fill(false);
		KeyCodesPressed.clear();
		KeyCodesReleased.clear();

		MousePressed.fill(false);
		MouseReleased.fill(false);
		MouseDelta = glm::vec2(0.0f);
		MouseWheel = glm::vec2(0.0f);

		for (auto& [gamepadID, state] : Gamepads)
		{
			state.Pressed.fill(false);
			state.Released.fill(false);
		}
	}

	void InputImplementation::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& keyEvent)
		{
			const size_t scanIndex = static_cast<size_t>(keyEvent.GetScanCode());

			if (scanIndex < KeysDown.size())
			{
				if (!keyEvent.IsRepeat() && !KeysDown[scanIndex])
				{
					KeysPressed[scanIndex] = true;
				}

				KeysDown[scanIndex] = true;
			}

			if (!keyEvent.IsRepeat() && !KeyCodesDown.contains(keyEvent.GetKeyCode()))
			{
				KeyCodesPressed.insert(keyEvent.GetKeyCode());
			}

			KeyCodesDown.insert(keyEvent.GetKeyCode());

			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& keyEvent)
		{
			const size_t scanIndex = static_cast<size_t>(keyEvent.GetScanCode());

			if (scanIndex < KeysDown.size())
			{
				KeysDown[scanIndex] = false;
				KeysReleased[scanIndex] = true;
			}

			KeyCodesDown.erase(keyEvent.GetKeyCode());
			KeyCodesReleased.insert(keyEvent.GetKeyCode());

			return false;
		});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& mouseEvent)
		{
			MousePosition = { mouseEvent.GetX(), mouseEvent.GetY() };
			MouseDelta.x += mouseEvent.GetDeltaX();
			MouseDelta.y += mouseEvent.GetDeltaY();

			return false;
		});

		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& mouseEvent)
		{
			MouseWheel.x += mouseEvent.GetXOffset();
			MouseWheel.y += mouseEvent.GetYOffset();

			return false;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& mouseEvent)
		{
			const size_t index = static_cast<size_t>(mouseEvent.GetMouseButton());

			if (index < MouseDown.size())
			{
				MouseDown[index] = true;
				MousePressed[index] = true;
			}

			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& mouseEvent)
		{
			const size_t index = static_cast<size_t>(mouseEvent.GetMouseButton());

			if (index < MouseDown.size())
			{
				MouseDown[index] = false;
				MouseReleased[index] = true;
			}

			return false;
		});

		dispatcher.Dispatch<GamepadConnectedEvent>([this](GamepadConnectedEvent& gamepadEvent)
		{
			OpenGamepad(gamepadEvent.GetGamepadID());

			return false;
		});

		dispatcher.Dispatch<GamepadDisconnectedEvent>([this](GamepadDisconnectedEvent& gamepadEvent)
		{
			CloseGamepad(gamepadEvent.GetGamepadID());

			return false;
		});

		dispatcher.Dispatch<GamepadButtonPressedEvent>([this](GamepadButtonPressedEvent& gamepadEvent)
		{
			GamepadState* state = FindGamepad(gamepadEvent.GetGamepadID());
			const int32_t index = static_cast<int32_t>(gamepadEvent.GetButton());

			if (state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT))
			{
				state->Down[index] = true;
				state->Pressed[index] = true;
			}

			return false;
		});

		dispatcher.Dispatch<GamepadButtonReleasedEvent>([this](GamepadButtonReleasedEvent& gamepadEvent)
		{
			GamepadState* state = FindGamepad(gamepadEvent.GetGamepadID());
			const int32_t index = static_cast<int32_t>(gamepadEvent.GetButton());

			if (state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT))
			{
				state->Down[index] = false;
				state->Released[index] = true;
			}

			return false;
		});

		dispatcher.Dispatch<GamepadAxisMovedEvent>([this](GamepadAxisMovedEvent& gamepadEvent)
		{
			GamepadState* state = FindGamepad(gamepadEvent.GetGamepadID());
			const int32_t index = static_cast<int32_t>(gamepadEvent.GetAxis());

			if (state && index >= 0 && index < static_cast<int32_t>(GamepadAxis::COUNT))
			{
				state->Axes[index] = gamepadEvent.GetValue();
			}

			return false;
		});

		dispatcher.Dispatch<WindowLostFocusEvent>([this](WindowLostFocusEvent&)
		{
			ClearHeldState();

			// Release a captured cursor while unfocused; intent is preserved in Cursor
			if (WindowHandle && Cursor == CursorMode::Captured)
			{
				WindowHandle->SetCursorMode(CursorMode::Normal);
			}

			return false;
		});

		dispatcher.Dispatch<WindowFocusEvent>([this](WindowFocusEvent&)
		{
			if (WindowHandle)
			{
				WindowHandle->SetCursorMode(Cursor);
			}

			return false;
		});
	}

	void InputImplementation::ClearHeldState()
	{
		// Keyboard and mouse only: the OS delivers their releases to whichever window gains focus, so held state would stick. Gamepads are not focus scoped and stay live
		KeysDown.fill(false);
		KeyCodesDown.clear();
		MouseDown.fill(false);
	}

	void InputImplementation::OpenGamepad(GamepadID gamepadID)
	{
		if (Gamepads.contains(gamepadID))
		{
			return;
		}

		SDL_Gamepad* handle = SDL_OpenGamepad(gamepadID);

		if (!handle)
		{
			IG_CORE_WARN("Failed to open gamepad {}: {}", gamepadID, SDL_GetError());

			return;
		}

		GamepadState state{};
		state.Handle = handle;
		Gamepads.emplace(gamepadID, state);

		const char* name = SDL_GetGamepadName(handle);

		IG_CORE_INFO("Gamepad connected: {} (id {})", name ? name : "", gamepadID);
	}

	void InputImplementation::CloseGamepad(GamepadID gamepadID)
	{
		auto it = Gamepads.find(gamepadID);

		if (it == Gamepads.end())
		{
			return;
		}

		if (it->second.Handle)
		{
			SDL_CloseGamepad(it->second.Handle);
		}

		Gamepads.erase(it);

		IG_CORE_INFO("Gamepad disconnected (id {})", gamepadID);
	}

	const InputImplementation::GamepadState* InputImplementation::FindGamepad(GamepadID gamepadID) const
	{
		auto it = Gamepads.find(gamepadID);

		return it != Gamepads.end() ? &it->second : nullptr;
	}

	InputImplementation::GamepadState* InputImplementation::FindGamepad(GamepadID gamepadID)
	{
		auto it = Gamepads.find(gamepadID);

		return it != Gamepads.end() ? &it->second : nullptr;
	}

	//-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------//

	Input::Input() : m_Implementation(std::make_unique<InputImplementation>())
	{

	}

	Input::~Input() = default;

	bool Input::IsKeyDown(ScanCode scanCode) const
	{
		const size_t index = static_cast<size_t>(scanCode);

		return index < m_Implementation->KeysDown.size() && m_Implementation->KeysDown[index];
	}

	bool Input::IsKeyPressed(ScanCode scanCode) const
	{
		const size_t index = static_cast<size_t>(scanCode);

		return index < m_Implementation->KeysPressed.size() && m_Implementation->KeysPressed[index];
	}

	bool Input::IsKeyReleased(ScanCode scanCode) const
	{
		const size_t index = static_cast<size_t>(scanCode);

		return index < m_Implementation->KeysReleased.size() && m_Implementation->KeysReleased[index];
	}

	bool Input::IsKeyDown(KeyCode keyCode) const
	{
		return m_Implementation->KeyCodesDown.contains(keyCode);
	}

	bool Input::IsKeyPressed(KeyCode keyCode) const
	{
		return m_Implementation->KeyCodesPressed.contains(keyCode);
	}

	bool Input::IsKeyReleased(KeyCode keyCode) const
	{
		return m_Implementation->KeyCodesReleased.contains(keyCode);
	}

	glm::vec2 Input::GetMousePosition() const
	{
		return m_Implementation->MousePosition;
	}

	glm::vec2 Input::GetMouseDelta() const
	{
		return m_Implementation->MouseDelta;
	}

	glm::vec2 Input::GetMouseWheel() const
	{
		return m_Implementation->MouseWheel;
	}

	bool Input::IsMouseButtonDown(MouseCode button) const
	{
		const size_t index = static_cast<size_t>(button);

		return index < m_Implementation->MouseDown.size() && m_Implementation->MouseDown[index];
	}

	bool Input::IsMouseButtonPressed(MouseCode button) const
	{
		const size_t index = static_cast<size_t>(button);

		return index < m_Implementation->MousePressed.size() && m_Implementation->MousePressed[index];
	}

	bool Input::IsMouseButtonReleased(MouseCode button) const
	{
		const size_t index = static_cast<size_t>(button);

		return index < m_Implementation->MouseReleased.size() && m_Implementation->MouseReleased[index];
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		m_Implementation->Cursor = mode;

		if (m_Implementation->WindowHandle)
		{
			m_Implementation->WindowHandle->SetCursorMode(mode);
		}
	}

	CursorMode Input::GetCursorMode() const
	{
		return m_Implementation->Cursor;
	}

	void Input::SetTextInputEnabled(bool enabled)
	{
		if (m_Implementation->WindowHandle)
		{
			m_Implementation->WindowHandle->SetTextInputEnabled(enabled);
		}
	}

	bool Input::IsGamepadConnected(GamepadID gamepadID) const
	{
		return m_Implementation->Gamepads.contains(gamepadID);
	}

	std::vector<GamepadID> Input::GetConnectedGamepads() const
	{
		std::vector<GamepadID> gamepadIDs;
		gamepadIDs.reserve(m_Implementation->Gamepads.size());

		for (const auto& [gamepadID, state] : m_Implementation->Gamepads)
		{
			gamepadIDs.push_back(gamepadID);
		}

		return gamepadIDs;
	}

	std::string Input::GetGamepadName(GamepadID gamepadID) const
	{
		const InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);

		if (!state || !state->Handle)
		{
			return {};
		}

		const char* name = SDL_GetGamepadName(state->Handle);

		return name ? name : std::string{};
	}

	bool Input::IsGamepadButtonDown(GamepadID gamepadID, GamepadButton button) const
	{
		const InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(button);

		return state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT) && state->Down[index];
	}

	bool Input::IsGamepadButtonPressed(GamepadID gamepadID, GamepadButton button) const
	{
		const InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(button);

		return state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT) && state->Pressed[index];
	}

	bool Input::IsGamepadButtonReleased(GamepadID gamepadID, GamepadButton button) const
	{
		const InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(button);

		return state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT) && state->Released[index];
	}

	float Input::GetGamepadAxis(GamepadID gamepadID, GamepadAxis axis) const
	{
		switch (axis)
		{
			case GamepadAxis::LEFTX: return GetGamepadStick(gamepadID, GamepadStick::Left).x;
			case GamepadAxis::LEFTY: return GetGamepadStick(gamepadID, GamepadStick::Left).y;
			case GamepadAxis::RIGHTX: return GetGamepadStick(gamepadID, GamepadStick::Right).x;
			case GamepadAxis::RIGHTY: return GetGamepadStick(gamepadID, GamepadStick::Right).y;
			case GamepadAxis::LEFT_TRIGGER:
			case GamepadAxis::RIGHT_TRIGGER: return ApplyThresholdDeadzone(GetGamepadAxisRaw(gamepadID, axis), m_Implementation->TriggerDeadzone);
			default: return 0.0f;
		}
	}

	float Input::GetGamepadAxisRaw(GamepadID gamepadID, GamepadAxis axis) const
	{
		const InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(axis);

		if (!state || index < 0 || index >= static_cast<int32_t>(GamepadAxis::COUNT))
		{
			return 0.0f;
		}

		return state->Axes[index];
	}

	glm::vec2 Input::GetGamepadStick(GamepadID gamepadID, GamepadStick stick) const
	{
		const InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);

		if (!state)
		{
			return glm::vec2(0.0f);
		}

		const bool left = stick == GamepadStick::Left;
		const float x = state->Axes[static_cast<size_t>(left ? GamepadAxis::LEFTX : GamepadAxis::RIGHTX)];
		const float y = state->Axes[static_cast<size_t>(left ? GamepadAxis::LEFTY : GamepadAxis::RIGHTY)];

		return ApplyRadialDeadzone(x, y, m_Implementation->StickDeadzone);
	}

	bool Input::SetGamepadRumble(GamepadID gamepadID, float lowFrequency, float highFrequency, uint32_t durationMilliseconds)
	{
		InputImplementation::GamepadState* state = m_Implementation->FindGamepad(gamepadID);

		if (!state || !state->Handle)
		{
			return false;
		}

		const uint16_t low = static_cast<uint16_t>(std::clamp(lowFrequency, 0.0f, 1.0f) * 65535.0f);
		const uint16_t high = static_cast<uint16_t>(std::clamp(highFrequency, 0.0f, 1.0f) * 65535.0f);

		return SDL_RumbleGamepad(state->Handle, low, high, durationMilliseconds);
	}

	float Input::GetStickDeadzone() const
	{
		return m_Implementation->StickDeadzone;
	}

	void Input::SetStickDeadzone(float deadzone)
	{
		if (deadzone < 0.0f || deadzone >= 1.0f)
		{
			IG_CORE_WARN("Invalid stick deadzone ({}), ignoring", deadzone);

			return;
		}

		m_Implementation->StickDeadzone = deadzone;
	}

	float Input::GetTriggerDeadzone() const
	{
		return m_Implementation->TriggerDeadzone;
	}

	void Input::SetTriggerDeadzone(float deadzone)
	{
		if (deadzone < 0.0f || deadzone >= 1.0f)
		{
			IG_CORE_WARN("Invalid trigger deadzone ({}), ignoring", deadzone);

			return;
		}

		m_Implementation->TriggerDeadzone = deadzone;
	}
}