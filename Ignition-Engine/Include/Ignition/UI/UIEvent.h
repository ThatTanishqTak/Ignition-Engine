#pragma once

namespace Ignition
{
	namespace UI
	{
		class Element;

		// Capture runs root->target, bubble runs target->root
		enum class UIEventPhase
		{
			Capture = 0,
			Bubble
		};

		enum class UIEventType
		{
			None = 0,
			PointerEnter,
			PointerLeave,
			PointerMove,
			PointerDown,
			PointerUp,
			Scroll,
			KeyDown,
			KeyUp,
			TextInput,
			TextEditing,
			FocusGained,
			FocusLost
		};

		class UIEvent
		{
		public:
			UIEvent() = default;
			virtual ~UIEvent() = default;

			UIEventType GetType() const { return m_Type; }
			UIEventPhase GetPhase() const { return m_Phase; }

			bool IsHandled() const { return m_Handled; }
			void SetHandled(bool handled = true) { m_Handled = handled; }

		protected:
			UIEventType m_Type = UIEventType::None;
			UIEventPhase m_Phase = UIEventPhase::Bubble;
			bool m_Handled = false;
		};
	}
}