#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Events/GamepadCodes.h"
#include "Ignition/Events/KeyCodes.h"
#include "Ignition/Events/MouseCodes.h"
#include "Ignition/Events/ScanCodes.h"
#include "Ignition/Input/Input.h"

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace Ignition
{
	class ActionMap
	{
	public:
		static constexpr GamepadID AnyGamepad = 0xFFFFFFFFu;

		class ButtonAction
		{
		public:
			IGNITION_API ButtonAction& Bind(ScanCode scanCode);
			IGNITION_API ButtonAction& Bind(KeyCode keyCode);
			IGNITION_API ButtonAction& Bind(MouseCode button);
			IGNITION_API ButtonAction& Bind(GamepadButton button, GamepadID gamepadID = AnyGamepad);
			IGNITION_API ButtonAction& ClearBindings();

		private:
			friend class ActionMap;

			struct GamepadBinding
			{
				GamepadButton Button = GamepadButton::INVALID;
				GamepadID Gamepad = AnyGamepad;
			};

			using Binding = std::variant<ScanCode, KeyCode, MouseCode, GamepadBinding>;

			std::vector<Binding> m_Bindings;
		};

		class Axis1DAction
		{
		public:
			IGNITION_API Axis1DAction& BindKeys(ScanCode negative, ScanCode positive);
			IGNITION_API Axis1DAction& BindAxis(GamepadAxis axis, GamepadID gamepadID = AnyGamepad);
			IGNITION_API Axis1DAction& ClearBindings();

		private:
			friend class ActionMap;

			struct KeyPairBinding
			{
				ScanCode Negative = ScanCode::UNKNOWN;
				ScanCode Positive = ScanCode::UNKNOWN;
			};

			struct AxisBinding
			{
				GamepadAxis Axis = GamepadAxis::INVALID;
				GamepadID Gamepad = AnyGamepad;
			};

			using Binding = std::variant<KeyPairBinding, AxisBinding>;

			std::vector<Binding> m_Bindings;
		};

		class Axis2DAction
		{
		public:
			IGNITION_API Axis2DAction& BindKeys(ScanCode up, ScanCode down, ScanCode left, ScanCode right);
			IGNITION_API Axis2DAction& BindStick(GamepadStick stick, GamepadID gamepadID = AnyGamepad);
			IGNITION_API Axis2DAction& ClearBindings();

		private:
			friend class ActionMap;

			struct CompositeBinding
			{
				ScanCode Up = ScanCode::UNKNOWN;
				ScanCode Down = ScanCode::UNKNOWN;
				ScanCode Left = ScanCode::UNKNOWN;
				ScanCode Right = ScanCode::UNKNOWN;
			};

			struct StickBinding
			{
				GamepadStick Stick = GamepadStick::Left;
				GamepadID Gamepad = AnyGamepad;
			};

			using Binding = std::variant<CompositeBinding, StickBinding>;

			std::vector<Binding> m_Bindings;
		};

		IGNITION_API explicit ActionMap(const Input* input);

		IGNITION_API void SetEnabled(bool enabled);
		IGNITION_API bool IsEnabled() const;

		IGNITION_API ButtonAction& AddButton(const std::string& name);
		IGNITION_API Axis1DAction& AddAxis1D(const std::string& name);
		IGNITION_API Axis2DAction& AddAxis2D(const std::string& name);

		IGNITION_API bool IsHeld(const std::string& name) const;
		IGNITION_API bool IsPressed(const std::string& name) const;
		IGNITION_API bool IsReleased(const std::string& name) const;
		IGNITION_API float GetAxis1D(const std::string& name) const;
		IGNITION_API Float2 GetAxis2D(const std::string& name) const;

	private:
		enum class ButtonQuery
		{
			Held = 0,
			Pressed,
			Released
		};

		bool EvaluateButton(const std::string& name, ButtonQuery query) const;

	private:
		const Input* m_Input = nullptr;
		bool m_Enabled = true;

		std::unordered_map<std::string, ButtonAction> m_Buttons;
		std::unordered_map<std::string, Axis1DAction> m_Axes1D;
		std::unordered_map<std::string, Axis2DAction> m_Axes2D;
	};
}