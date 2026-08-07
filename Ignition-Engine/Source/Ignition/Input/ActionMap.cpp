#include "Ignition/Input/ActionMap.h"

#include <cmath>

namespace Ignition
{
	ActionMap::ButtonAction& ActionMap::ButtonAction::Bind(ScanCode scanCode)
	{
		m_Bindings.emplace_back(scanCode);

		return *this;
	}

	ActionMap::ButtonAction& ActionMap::ButtonAction::Bind(KeyCode keyCode)
	{
		m_Bindings.emplace_back(keyCode);

		return *this;
	}

	ActionMap::ButtonAction& ActionMap::ButtonAction::Bind(MouseCode button)
	{
		m_Bindings.emplace_back(button);

		return *this;
	}

	ActionMap::ButtonAction& ActionMap::ButtonAction::Bind(GamepadButton button, GamepadID gamepadID)
	{
		m_Bindings.emplace_back(GamepadBinding{ button, gamepadID });

		return *this;
	}

	ActionMap::ButtonAction& ActionMap::ButtonAction::ClearBindings()
	{
		m_Bindings.clear();

		return *this;
	}

	ActionMap::Axis1DAction& ActionMap::Axis1DAction::BindKeys(ScanCode negative, ScanCode positive)
	{
		m_Bindings.emplace_back(KeyPairBinding{ negative, positive });

		return *this;
	}

	ActionMap::Axis1DAction& ActionMap::Axis1DAction::BindAxis(GamepadAxis axis, GamepadID gamepadID)
	{
		m_Bindings.emplace_back(AxisBinding{ axis, gamepadID });

		return *this;
	}

	ActionMap::Axis1DAction& ActionMap::Axis1DAction::ClearBindings()
	{
		m_Bindings.clear();

		return *this;
	}

	ActionMap::Axis2DAction& ActionMap::Axis2DAction::BindKeys(ScanCode up, ScanCode down, ScanCode left, ScanCode right)
	{
		m_Bindings.emplace_back(CompositeBinding{ up, down, left, right });

		return *this;
	}

	ActionMap::Axis2DAction& ActionMap::Axis2DAction::BindStick(GamepadStick stick, GamepadID gamepadID)
	{
		m_Bindings.emplace_back(StickBinding{ stick, gamepadID });

		return *this;
	}

	ActionMap::Axis2DAction& ActionMap::Axis2DAction::ClearBindings()
	{
		m_Bindings.clear();

		return *this;
	}

	ActionMap::ActionMap(const Input* input) : m_Input(input)
	{

	}

	void ActionMap::SetEnabled(bool enabled)
	{
		m_Enabled = enabled;
	}

	bool ActionMap::IsEnabled() const
	{
		return m_Enabled;
	}

	ActionMap::ButtonAction& ActionMap::AddButton(const std::string& name)
	{
		return m_Buttons[name];
	}

	ActionMap::Axis1DAction& ActionMap::AddAxis1D(const std::string& name)
	{
		return m_Axes1D[name];
	}

	ActionMap::Axis2DAction& ActionMap::AddAxis2D(const std::string& name)
	{
		return m_Axes2D[name];
	}

	bool ActionMap::IsHeld(const std::string& name) const
	{
		return EvaluateButton(name, ButtonQuery::Held);
	}

	bool ActionMap::IsPressed(const std::string& name) const
	{
		return EvaluateButton(name, ButtonQuery::Pressed);
	}

	bool ActionMap::IsReleased(const std::string& name) const
	{
		return EvaluateButton(name, ButtonQuery::Released);
	}

	bool ActionMap::EvaluateButton(const std::string& name, ButtonQuery query) const
	{
		if (!m_Enabled || !m_Input)
		{
			return false;
		}

		const auto it = m_Buttons.find(name);

		if (it == m_Buttons.end())
		{
			return false;
		}

		const auto checkGamepad = [&](GamepadButton button, GamepadID gamepadID)
		{
			switch (query)
			{
				case ButtonQuery::Held: return m_Input->IsGamepadButtonDown(gamepadID, button);
				case ButtonQuery::Pressed: return m_Input->IsGamepadButtonPressed(gamepadID, button);
				case ButtonQuery::Released: return m_Input->IsGamepadButtonReleased(gamepadID, button);
			}

			return false;
		};

		for (const ButtonAction::Binding& binding : it->second.m_Bindings)
		{
			const bool active = std::visit([&](const auto& bound) -> bool
			{
				using T = std::decay_t<decltype(bound)>;

				if constexpr (std::is_same_v<T, ScanCode> || std::is_same_v<T, KeyCode>)
				{
					switch (query)
					{
						case ButtonQuery::Held: return m_Input->IsKeyDown(bound);
						case ButtonQuery::Pressed: return m_Input->IsKeyPressed(bound);
						case ButtonQuery::Released: return m_Input->IsKeyReleased(bound);
					}

					return false;
				}
				else if constexpr (std::is_same_v<T, MouseCode>)
				{
					switch (query)
					{
						case ButtonQuery::Held: return m_Input->IsMouseButtonDown(bound);
						case ButtonQuery::Pressed: return m_Input->IsMouseButtonPressed(bound);
						case ButtonQuery::Released: return m_Input->IsMouseButtonReleased(bound);
					}

					return false;
				}
				else
				{
					if (bound.Gamepad != AnyGamepad)
					{
						return checkGamepad(bound.Button, bound.Gamepad);
					}

					for (GamepadID gamepadID : m_Input->GetConnectedGamepads())
					{
						if (checkGamepad(bound.Button, gamepadID))
						{
							return true;
						}
					}

					return false;
				}
			}, binding);

			if (active)
			{
				return true;
			}
		}

		return false;
	}

	float ActionMap::GetAxis1D(const std::string& name) const
	{
		if (!m_Enabled || !m_Input)
		{
			return 0.0f;
		}

		const auto it = m_Axes1D.find(name);

		if (it == m_Axes1D.end())
		{
			return 0.0f;
		}

		float best = 0.0f;

		for (const Axis1DAction::Binding& binding : it->second.m_Bindings)
		{
			const float value = std::visit([&](const auto& bound) -> float
			{
				using T = std::decay_t<decltype(bound)>;

				if constexpr (std::is_same_v<T, Axis1DAction::KeyPairBinding>)
				{
					return (m_Input->IsKeyDown(bound.Positive) ? 1.0f : 0.0f) - (m_Input->IsKeyDown(bound.Negative) ? 1.0f : 0.0f);
				}
				else
				{
					if (bound.Gamepad != AnyGamepad)
					{
						return m_Input->GetGamepadAxis(bound.Gamepad, bound.Axis);
					}

					float largest = 0.0f;

					for (GamepadID gamepadID : m_Input->GetConnectedGamepads())
					{
						const float candidate = m_Input->GetGamepadAxis(gamepadID, bound.Axis);

						if (std::abs(candidate) > std::abs(largest))
						{
							largest = candidate;
						}
					}

					return largest;
				}
			}, binding);

			if (std::abs(value) > std::abs(best))
			{
				best = value;
			}
		}

		return best;
	}

	Float2 ActionMap::GetAxis2D(const std::string& name) const
	{
		if (!m_Enabled || !m_Input)
		{
			return {};
		}

		const auto it = m_Axes2D.find(name);

		if (it == m_Axes2D.end())
		{
			return {};
		}

		Float2 best{};
		float bestMagnitudeSquared = 0.0f;

		for (const Axis2DAction::Binding& binding : it->second.m_Bindings)
		{
			const Float2 value = std::visit([&](const auto& bound) -> Float2
			{
				using T = std::decay_t<decltype(bound)>;

				if constexpr (std::is_same_v<T, Axis2DAction::CompositeBinding>)
				{
					Float2 composite{};
					composite.X = (m_Input->IsKeyDown(bound.Right) ? 1.0f : 0.0f) - (m_Input->IsKeyDown(bound.Left) ? 1.0f : 0.0f);
					composite.Y = (m_Input->IsKeyDown(bound.Up) ? 1.0f : 0.0f) - (m_Input->IsKeyDown(bound.Down) ? 1.0f : 0.0f);

					// Normalize so diagonals are not faster than cardinals
					const float length = std::sqrt(composite.X * composite.X + composite.Y * composite.Y);

					if (length > 1.0f)
					{
						composite.X /= length;
						composite.Y /= length;
					}

					return composite;
				}
				else
				{
					const auto readStick = [&](GamepadID gamepadID) -> Float2
					{
						const Float2 stick = m_Input->GetGamepadStick(gamepadID, bound.Stick);

						// SDL sticks are Y down-positive; convert to the map's Y up-positive convention
						return { stick.X, -stick.Y };
					};

					if (bound.Gamepad != AnyGamepad)
					{
						return readStick(bound.Gamepad);
					}

					Float2 largest{};
					float largestMagnitudeSquared = 0.0f;

					for (GamepadID gamepadID : m_Input->GetConnectedGamepads())
					{
						const Float2 candidate = readStick(gamepadID);
						const float magnitudeSquared = candidate.X * candidate.X + candidate.Y * candidate.Y;

						if (magnitudeSquared > largestMagnitudeSquared)
						{
							largestMagnitudeSquared = magnitudeSquared;
							largest = candidate;
						}
					}

					return largest;
				}
			}, binding);

			const float magnitudeSquared = value.X * value.X + value.Y * value.Y;

			if (magnitudeSquared > bestMagnitudeSquared)
			{
				bestMagnitudeSquared = magnitudeSquared;
				best = value;
			}
		}

		return best;
	}
}