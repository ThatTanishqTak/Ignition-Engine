#include "Ignition/Input/Input.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/Event.h"
#include "Ignition/Events/GamepadEvent.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"
#include "Ignition/Window/Window.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace
{
	Ignition::Float2 ApplyRadialDeadzone(float x, float y, float deadzone)
	{
		const float magnitude = std::sqrt(x * x + y * y);

		if (magnitude < deadzone)
		{
			return {};
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
	Input::Input() = default;
	Input::~Input() = default;

	void Input::Initialize(Window* window)
	{
		IG_CORE_INFO("------- INITIALIZING INPUT -------");

		m_Window = window;

		IG_CORE_INFO("------- INPUT INITIALIZED -------");
	}

	void Input::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN INPUT -------");

		for (auto& [gamepadID, state] : m_Gamepads)
		{
			if (state.Handle)
			{
				SDL_CloseGamepad(state.Handle);
			}
		}

		m_Gamepads.clear();
		m_Window = nullptr;

		IG_CORE_INFO("------- INPUT SHUTDOWN COMPLETE -------");
	}

	void Input::NewFrame()
	{
		m_KeysPressed.fill(false);
		m_KeysReleased.fill(false);
		m_KeyCodesPressed.clear();
		m_KeyCodesReleased.clear();

		m_MousePressed.fill(false);
		m_MouseReleased.fill(false);
		m_MouseDelta = {};
		m_MouseWheel = {};

		for (auto& [gamepadID, state] : m_Gamepads)
		{
			state.Pressed.fill(false);
			state.Released.fill(false);
		}
	}

	void Input::OnEvent(Event& event)
	{
		EventDispatcher dispatcher(event);

		dispatcher.Dispatch<KeyPressedEvent>([this](KeyPressedEvent& keyEvent)
		{
			const size_t scanIndex = static_cast<size_t>(keyEvent.GetScanCode());

			if (scanIndex < m_KeysDown.size())
			{
				if (!keyEvent.IsRepeat() && !m_KeysDown[scanIndex])
				{
					m_KeysPressed[scanIndex] = true;
				}

				m_KeysDown[scanIndex] = true;
			}

			if (!keyEvent.IsRepeat() && !m_KeyCodesDown.contains(keyEvent.GetKeyCode()))
			{
				m_KeyCodesPressed.insert(keyEvent.GetKeyCode());
			}

			m_KeyCodesDown.insert(keyEvent.GetKeyCode());

			return false;
		});

		dispatcher.Dispatch<KeyReleasedEvent>([this](KeyReleasedEvent& keyEvent)
		{
			const size_t scanIndex = static_cast<size_t>(keyEvent.GetScanCode());

			if (scanIndex < m_KeysDown.size())
			{
				m_KeysDown[scanIndex] = false;
				m_KeysReleased[scanIndex] = true;
			}

			m_KeyCodesDown.erase(keyEvent.GetKeyCode());
			m_KeyCodesReleased.insert(keyEvent.GetKeyCode());

			return false;
		});

		dispatcher.Dispatch<MouseMovedEvent>([this](MouseMovedEvent& mouseEvent)
		{
			m_MousePosition = { mouseEvent.GetX(), mouseEvent.GetY() };
			m_MouseDelta.X += mouseEvent.GetDeltaX();
			m_MouseDelta.Y += mouseEvent.GetDeltaY();

			return false;
		});

		dispatcher.Dispatch<MouseScrolledEvent>([this](MouseScrolledEvent& mouseEvent)
		{
			m_MouseWheel.X += mouseEvent.GetXOffset();
			m_MouseWheel.Y += mouseEvent.GetYOffset();

			return false;
		});

		dispatcher.Dispatch<MouseButtonPressedEvent>([this](MouseButtonPressedEvent& mouseEvent)
		{
			const size_t index = static_cast<size_t>(mouseEvent.GetMouseButton());

			if (index < m_MouseDown.size())
			{
				m_MouseDown[index] = true;
				m_MousePressed[index] = true;
			}

			return false;
		});

		dispatcher.Dispatch<MouseButtonReleasedEvent>([this](MouseButtonReleasedEvent& mouseEvent)
		{
			const size_t index = static_cast<size_t>(mouseEvent.GetMouseButton());

			if (index < m_MouseDown.size())
			{
				m_MouseDown[index] = false;
				m_MouseReleased[index] = true;
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

			// Release a captured cursor while unfocused; intent is preserved in m_CursorMode
			if (m_Window && m_CursorMode == CursorMode::Captured)
			{
				m_Window->SetCursorMode(CursorMode::Normal);
			}

			return false;
		});

		dispatcher.Dispatch<WindowFocusEvent>([this](WindowFocusEvent&)
		{
			if (m_Window)
			{
				m_Window->SetCursorMode(m_CursorMode);
			}

			return false;
		});
	}

	void Input::ClearHeldState()
	{
		// Keyboard and mouse only: the OS delivers their releases to whichever window
		// gains focus, so held state would otherwise stick. Gamepads are not focus
		// scoped and keep streaming events, so their state stays live.
		m_KeysDown.fill(false);
		m_KeyCodesDown.clear();
		m_MouseDown.fill(false);
	}

	bool Input::IsKeyDown(ScanCode scanCode) const
	{
		const size_t index = static_cast<size_t>(scanCode);

		return index < m_KeysDown.size() && m_KeysDown[index];
	}

	bool Input::IsKeyPressed(ScanCode scanCode) const
	{
		const size_t index = static_cast<size_t>(scanCode);

		return index < m_KeysPressed.size() && m_KeysPressed[index];
	}

	bool Input::IsKeyReleased(ScanCode scanCode) const
	{
		const size_t index = static_cast<size_t>(scanCode);

		return index < m_KeysReleased.size() && m_KeysReleased[index];
	}

	bool Input::IsKeyDown(KeyCode keyCode) const
	{
		return m_KeyCodesDown.contains(keyCode);
	}

	bool Input::IsKeyPressed(KeyCode keyCode) const
	{
		return m_KeyCodesPressed.contains(keyCode);
	}

	bool Input::IsKeyReleased(KeyCode keyCode) const
	{
		return m_KeyCodesReleased.contains(keyCode);
	}

	Float2 Input::GetMousePosition() const
	{
		return m_MousePosition;
	}

	Float2 Input::GetMouseDelta() const
	{
		return m_MouseDelta;
	}

	Float2 Input::GetMouseWheel() const
	{
		return m_MouseWheel;
	}

	bool Input::IsMouseButtonDown(MouseCode button) const
	{
		const size_t index = static_cast<size_t>(button);

		return index < m_MouseDown.size() && m_MouseDown[index];
	}

	bool Input::IsMouseButtonPressed(MouseCode button) const
	{
		const size_t index = static_cast<size_t>(button);

		return index < m_MousePressed.size() && m_MousePressed[index];
	}

	bool Input::IsMouseButtonReleased(MouseCode button) const
	{
		const size_t index = static_cast<size_t>(button);

		return index < m_MouseReleased.size() && m_MouseReleased[index];
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		m_CursorMode = mode;

		if (m_Window)
		{
			m_Window->SetCursorMode(mode);
		}
	}

	CursorMode Input::GetCursorMode() const
	{
		return m_CursorMode;
	}

	void Input::SetTextInputEnabled(bool enabled)
	{
		if (m_Window)
		{
			m_Window->SetTextInputEnabled(enabled);
		}
	}

	bool Input::IsGamepadConnected(GamepadID gamepadID) const
	{
		return m_Gamepads.contains(gamepadID);
	}

	std::vector<GamepadID> Input::GetConnectedGamepads() const
	{
		std::vector<GamepadID> gamepadIDs;
		gamepadIDs.reserve(m_Gamepads.size());

		for (const auto& [gamepadID, state] : m_Gamepads)
		{
			gamepadIDs.push_back(gamepadID);
		}

		return gamepadIDs;
	}

	std::string Input::GetGamepadName(GamepadID gamepadID) const
	{
		const GamepadState* state = FindGamepad(gamepadID);

		if (!state || !state->Handle)
		{
			return {};
		}

		const char* name = SDL_GetGamepadName(state->Handle);

		return name ? name : std::string{};
	}

	bool Input::IsGamepadButtonDown(GamepadID gamepadID, GamepadButton button) const
	{
		const GamepadState* state = FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(button);

		return state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT) && state->Down[index];
	}

	bool Input::IsGamepadButtonPressed(GamepadID gamepadID, GamepadButton button) const
	{
		const GamepadState* state = FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(button);

		return state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT) && state->Pressed[index];
	}

	bool Input::IsGamepadButtonReleased(GamepadID gamepadID, GamepadButton button) const
	{
		const GamepadState* state = FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(button);

		return state && index >= 0 && index < static_cast<int32_t>(GamepadButton::COUNT) && state->Released[index];
	}

	float Input::GetGamepadAxis(GamepadID gamepadID, GamepadAxis axis) const
	{
		switch (axis)
		{
			case GamepadAxis::LEFTX: return GetGamepadStick(gamepadID, GamepadStick::Left).X;
			case GamepadAxis::LEFTY: return GetGamepadStick(gamepadID, GamepadStick::Left).Y;
			case GamepadAxis::RIGHTX: return GetGamepadStick(gamepadID, GamepadStick::Right).X;
			case GamepadAxis::RIGHTY: return GetGamepadStick(gamepadID, GamepadStick::Right).Y;
			case GamepadAxis::LEFT_TRIGGER:
			case GamepadAxis::RIGHT_TRIGGER: return ApplyThresholdDeadzone(GetGamepadAxisRaw(gamepadID, axis), m_TriggerDeadzone);
			default: return 0.0f;
		}
	}

	float Input::GetGamepadAxisRaw(GamepadID gamepadID, GamepadAxis axis) const
	{
		const GamepadState* state = FindGamepad(gamepadID);
		const int32_t index = static_cast<int32_t>(axis);

		if (!state || index < 0 || index >= static_cast<int32_t>(GamepadAxis::COUNT))
		{
			return 0.0f;
		}

		return state->Axes[index];
	}

	Float2 Input::GetGamepadStick(GamepadID gamepadID, GamepadStick stick) const
	{
		const GamepadState* state = FindGamepad(gamepadID);

		if (!state)
		{
			return {};
		}

		const bool left = stick == GamepadStick::Left;
		const float x = state->Axes[static_cast<size_t>(left ? GamepadAxis::LEFTX : GamepadAxis::RIGHTX)];
		const float y = state->Axes[static_cast<size_t>(left ? GamepadAxis::LEFTY : GamepadAxis::RIGHTY)];

		return ApplyRadialDeadzone(x, y, m_StickDeadzone);
	}

	bool Input::SetGamepadRumble(GamepadID gamepadID, float lowFrequency, float highFrequency, uint32_t durationMilliseconds)
	{
		GamepadState* state = FindGamepad(gamepadID);

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
		return m_StickDeadzone;
	}

	void Input::SetStickDeadzone(float deadzone)
	{
		if (deadzone < 0.0f || deadzone >= 1.0f)
		{
			IG_CORE_WARN("Invalid stick deadzone ({}), ignoring", deadzone);

			return;
		}

		m_StickDeadzone = deadzone;
	}

	float Input::GetTriggerDeadzone() const
	{
		return m_TriggerDeadzone;
	}

	void Input::SetTriggerDeadzone(float deadzone)
	{
		if (deadzone < 0.0f || deadzone >= 1.0f)
		{
			IG_CORE_WARN("Invalid trigger deadzone ({}), ignoring", deadzone);

			return;
		}

		m_TriggerDeadzone = deadzone;
	}

	void Input::OpenGamepad(GamepadID gamepadID)
	{
		if (m_Gamepads.contains(gamepadID))
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
		m_Gamepads.emplace(gamepadID, state);

		IG_CORE_INFO("Gamepad connected: {} (id {})", GetGamepadName(gamepadID), gamepadID);
	}

	void Input::CloseGamepad(GamepadID gamepadID)
	{
		auto it = m_Gamepads.find(gamepadID);

		if (it == m_Gamepads.end())
		{
			return;
		}

		if (it->second.Handle)
		{
			SDL_CloseGamepad(it->second.Handle);
		}

		m_Gamepads.erase(it);

		IG_CORE_INFO("Gamepad disconnected (id {})", gamepadID);
	}

	const Input::GamepadState* Input::FindGamepad(GamepadID gamepadID) const
	{
		auto it = m_Gamepads.find(gamepadID);

		return it != m_Gamepads.end() ? &it->second : nullptr;
	}

	Input::GamepadState* Input::FindGamepad(GamepadID gamepadID)
	{
		auto it = m_Gamepads.find(gamepadID);

		return it != m_Gamepads.end() ? &it->second : nullptr;
	}
}