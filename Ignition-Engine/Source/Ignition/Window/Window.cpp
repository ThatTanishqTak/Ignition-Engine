#include "Ignition/Window/Window.h"

#include "Ignition/Window/WindowImplementation.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/EventQueue.h"
#include "Ignition/Events/GamepadEvent.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"

#include <SDL3/SDL.h>

namespace
{
	SDL_SystemCursor ToSDLSystemCursor(Ignition::CursorShape shape)
	{
		switch (shape)
		{
			case Ignition::CursorShape::TextInput:
				return SDL_SYSTEM_CURSOR_TEXT;
			case Ignition::CursorShape::Wait:
				return SDL_SYSTEM_CURSOR_WAIT;
			case Ignition::CursorShape::Crosshair:
				return SDL_SYSTEM_CURSOR_CROSSHAIR;
			case Ignition::CursorShape::Hand:
				return SDL_SYSTEM_CURSOR_POINTER;
			case Ignition::CursorShape::NotAllowed:
				return SDL_SYSTEM_CURSOR_NOT_ALLOWED;
			case Ignition::CursorShape::ResizeEW:
				return SDL_SYSTEM_CURSOR_EW_RESIZE;
			case Ignition::CursorShape::ResizeNS:
				return SDL_SYSTEM_CURSOR_NS_RESIZE;
			case Ignition::CursorShape::ResizeNESW:
				return SDL_SYSTEM_CURSOR_NESW_RESIZE;
			case Ignition::CursorShape::ResizeNWSE:
				return SDL_SYSTEM_CURSOR_NWSE_RESIZE;
			case Ignition::CursorShape::ResizeAll:
				return SDL_SYSTEM_CURSOR_MOVE;
			default:
				return SDL_SYSTEM_CURSOR_DEFAULT;
		}
	}
}

namespace Ignition
{
	Window::Window() : m_Implementation(std::make_unique<WindowImplementation>())
	{

	}

	Window::~Window() = default;

	void WindowImplementation::Initialize(const char* title, int width, int height)
	{
		IG_CORE_TRACE("Initializing Window");

		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
		{
			IG_CORE_CRITICAL("Failed to initialize SDL: {}", SDL_GetError());

			return;
		}

		SDLInitialized = true;
		SDLWindow = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (!SDLWindow)
		{
			IG_CORE_CRITICAL("Failed to create SDL window: {}", SDL_GetError());
			SDL_Quit();
			SDLInitialized = false;

			return;
		}

		IsOpen = true;

		IG_CORE_TRACE("Window Initialized");
	}

	void WindowImplementation::Shutdown()
	{
		IG_CORE_TRACE("Shutting Down Window");

		for (SDL_Cursor*& cursor : SystemCursors)
		{
			if (cursor)
			{
				SDL_DestroyCursor(cursor);
				cursor = nullptr;
			}
		}

		if (SDLWindow)
		{
			SDL_DestroyWindow(SDLWindow);
			SDLWindow = nullptr;
		}

		if (SDLInitialized)
		{
			SDL_Quit();
			SDLInitialized = false;
		}

		IsOpen = false;

		IG_CORE_TRACE("Window Shutdown Complete");
	}

	void WindowImplementation::PollEvents(EventQueue& eventQueue)
	{
		const SDL_WindowID windowID = SDLWindow ? SDL_GetWindowID(SDLWindow) : 0;

		KeyModifiers modifiers = static_cast<KeyModifiers>(SDL_GetModState());
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			if (RawCallback)
			{
				RawCallback(&event);
			}

			if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST && event.window.windowID != windowID)
			{
				continue;
			}

			switch (event.type)
			{
				case SDL_EVENT_QUIT:
					IsOpen = false;
					eventQueue.Push<WindowCloseEvent>();
					break;

				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					IsOpen = false;
					eventQueue.Push<WindowCloseEvent>();
					break;

				case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
					eventQueue.Push<WindowResizeEvent>(event.window.data1, event.window.data2);
					break;

				case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
					eventQueue.Push<WindowDisplayScaleChangedEvent>();
					break;

				case SDL_EVENT_WINDOW_DISPLAY_CHANGED:
					eventQueue.Push<WindowDisplayChangedEvent>(event.window.data1);
					break;

				case SDL_EVENT_WINDOW_MOVED:
					eventQueue.Push<WindowMovedEvent>(event.window.data1, event.window.data2);
					break;

				case SDL_EVENT_WINDOW_FOCUS_GAINED:
					eventQueue.Push<WindowFocusEvent>();
					break;

				case SDL_EVENT_WINDOW_FOCUS_LOST:
					eventQueue.Push<WindowLostFocusEvent>();
					break;

				case SDL_EVENT_WINDOW_MINIMIZED:
					eventQueue.Push<WindowMinimizedEvent>();
					break;

				case SDL_EVENT_WINDOW_MAXIMIZED:
					eventQueue.Push<WindowMaximizedEvent>();
					break;

				case SDL_EVENT_WINDOW_RESTORED:
					eventQueue.Push<WindowRestoredEvent>();
					break;

				case SDL_EVENT_WINDOW_SHOWN:
					eventQueue.Push<WindowShownEvent>();
					break;

				case SDL_EVENT_WINDOW_HIDDEN:
					eventQueue.Push<WindowHiddenEvent>();
					break;

				case SDL_EVENT_WINDOW_EXPOSED:
					eventQueue.Push<WindowExposedEvent>();
					break;

				case SDL_EVENT_WINDOW_OCCLUDED:
					eventQueue.Push<WindowOccludedEvent>();
					break;

				case SDL_EVENT_WINDOW_MOUSE_ENTER:
					eventQueue.Push<WindowMouseEnterEvent>();
					break;

				case SDL_EVENT_WINDOW_MOUSE_LEAVE:
					eventQueue.Push<WindowMouseLeaveEvent>();
					break;

				case SDL_EVENT_WINDOW_ENTER_FULLSCREEN:
					eventQueue.Push<WindowEnterFullscreenEvent>();
					break;

				case SDL_EVENT_WINDOW_LEAVE_FULLSCREEN:
					eventQueue.Push<WindowLeaveFullscreenEvent>();
					break;

				case SDL_EVENT_DROP_FILE:
					if (event.drop.data)
					{
						eventQueue.Push<FileDroppedEvent>(std::string(event.drop.data), event.drop.x, event.drop.y);
					}
					break;

				case SDL_EVENT_DROP_TEXT:
					if (event.drop.data)
					{
						eventQueue.Push<TextDroppedEvent>(std::string(event.drop.data), event.drop.x, event.drop.y);
					}
					break;

				case SDL_EVENT_TEXT_INPUT:
					if (event.text.text)
					{
						eventQueue.Push<TextInputEvent>(std::string(event.text.text));
					}
					break;

				case SDL_EVENT_TEXT_EDITING:
					if (event.edit.text)
					{
						eventQueue.Push<TextEditingEvent>(std::string(event.edit.text), event.edit.start, event.edit.length);
					}
					break;

				case SDL_EVENT_GAMEPAD_ADDED:
					eventQueue.Push<GamepadConnectedEvent>(event.gdevice.which);
					break;

				case SDL_EVENT_GAMEPAD_REMOVED:
					eventQueue.Push<GamepadDisconnectedEvent>(event.gdevice.which);
					break;

				case SDL_EVENT_GAMEPAD_REMAPPED:
					eventQueue.Push<GamepadRemappedEvent>(event.gdevice.which);
					break;

				case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
					eventQueue.Push<GamepadButtonPressedEvent>(event.gbutton.which, static_cast<GamepadButton>(event.gbutton.button));
					break;

				case SDL_EVENT_GAMEPAD_BUTTON_UP:
					eventQueue.Push<GamepadButtonReleasedEvent>(event.gbutton.which, static_cast<GamepadButton>(event.gbutton.button));
					break;

				case SDL_EVENT_GAMEPAD_AXIS_MOTION:
				{
					const GamepadAxis axis = static_cast<GamepadAxis>(event.gaxis.axis);
					const bool isTrigger = axis == GamepadAxis::LEFT_TRIGGER || axis == GamepadAxis::RIGHT_TRIGGER;

					float normalized = 0.0f;
					if (isTrigger)
					{
						normalized = static_cast<float>(event.gaxis.value) / 32767.0f;
					}
					else
					{
						normalized = static_cast<float>(event.gaxis.value) / (event.gaxis.value < 0 ? 32768.0f : 32767.0f);
					}

					eventQueue.Push<GamepadAxisMovedEvent>(event.gaxis.which, axis, normalized, event.gaxis.value);
					break;
				}

				case SDL_EVENT_KEY_DOWN:
					modifiers = static_cast<KeyModifiers>(event.key.mod);
					eventQueue.Push<KeyPressedEvent>(static_cast<KeyCode>(event.key.key), static_cast<ScanCode>(event.key.scancode), modifiers,
						event.key.repeat);
					break;

				case SDL_EVENT_KEY_UP:
					modifiers = static_cast<KeyModifiers>(event.key.mod);
					eventQueue.Push<KeyReleasedEvent>(static_cast<KeyCode>(event.key.key), static_cast<ScanCode>(event.key.scancode), modifiers);
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					eventQueue.Push<MouseButtonPressedEvent>(static_cast<MouseCode>(event.button.button), modifiers);
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP:
					eventQueue.Push<MouseButtonReleasedEvent>(static_cast<MouseCode>(event.button.button), modifiers);
					break;

				case SDL_EVENT_MOUSE_MOTION:
					eventQueue.Push<MouseMovedEvent>(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
					break;

				case SDL_EVENT_MOUSE_WHEEL:
					eventQueue.Push<MouseScrolledEvent>(event.wheel.x, event.wheel.y);
					break;

				default:
					break;
			}
		}
	}

	void Window::GetPixelSize(int& outWidth, int& outHeight) const
	{
		outWidth = 0;
		outHeight = 0;

		if (m_Implementation->SDLWindow)
		{
			SDL_GetWindowSizeInPixels(m_Implementation->SDLWindow, &outWidth, &outHeight);
		}
	}

	void Window::SetTextInputEnabled(bool enabled)
	{
		if (!m_Implementation->SDLWindow)
		{
			return;
		}

		if (enabled)
		{
			SDL_StartTextInput(m_Implementation->SDLWindow);
		}
		else
		{
			SDL_StopTextInput(m_Implementation->SDLWindow);
		}
	}

	// Called on every caret move so the IME candidate window follows the caret rather than parking in a corner
	void Window::SetTextInputArea(int x, int y, int width, int height, int cursorOffset)
	{
		if (!m_Implementation->SDLWindow)
		{
			return;
		}

		const SDL_Rect area{ x, y, width, height };

		if (!SDL_SetTextInputArea(m_Implementation->SDLWindow, &area, cursorOffset))
		{
			IG_CORE_WARN("Failed SDL_SetTextInputArea: {}", SDL_GetError());
		}
	}

	std::string Window::GetClipboardText() const
	{
		char* const text = SDL_GetClipboardText();

		if (!text)
		{
			return {};
		}

		std::string result(text);
		SDL_free(text);

		return result;
	}

	void Window::SetClipboardText(const char* text)
	{
		if (!SDL_SetClipboardText(text ? text : ""))
		{
			IG_CORE_WARN("Failed SDL_SetClipboardText: {}", SDL_GetError());
		}
	}

	void Window::SetCursorMode(CursorMode mode)
	{
		if (!m_Implementation->SDLWindow)
		{
			return;
		}

		m_Implementation->Cursor = mode;

		switch (mode)
		{
			case CursorMode::Normal:
				if (!SDL_SetWindowRelativeMouseMode(m_Implementation->SDLWindow, false))
				{
					IG_CORE_WARN("Failed SDL_SetWindowRelativeMouseMode: {}", SDL_GetError());
				}
				SDL_ShowCursor();
				break;

			case CursorMode::Hidden:
				if (!SDL_SetWindowRelativeMouseMode(m_Implementation->SDLWindow, false))
				{
					IG_CORE_WARN("Failed SDL_SetWindowRelativeMouseMode: {}", SDL_GetError());
				}
				SDL_HideCursor();
				break;

			case CursorMode::Captured:
				SDL_HideCursor();
				if (!SDL_SetWindowRelativeMouseMode(m_Implementation->SDLWindow, true))
				{
					IG_CORE_WARN("Failed SDL_SetWindowRelativeMouseMode: {}", SDL_GetError());
				}
				break;
		}
	}

	CursorMode Window::GetCursorMode() const
	{
		return m_Implementation->Cursor;
	}

	void Window::SetCursorShape(CursorShape shape)
	{
		if (!m_Implementation->SDLWindow || shape == CursorShape::Count || shape == m_Implementation->Shape)
		{
			return;
		}

		const std::size_t index = static_cast<std::size_t>(shape);

		if (!m_Implementation->SystemCursors[index])
		{
			m_Implementation->SystemCursors[index] = SDL_CreateSystemCursor(ToSDLSystemCursor(shape));

			if (!m_Implementation->SystemCursors[index])
			{
				IG_CORE_WARN("Failed SDL_CreateSystemCursor: {}", SDL_GetError());

				return;
			}
		}

		m_Implementation->Shape = shape;
		SDL_SetCursor(m_Implementation->SystemCursors[index]);
	}

	CursorShape Window::GetCursorShape() const
	{
		return m_Implementation->Shape;
	}

	bool Window::IsOpen() const
	{
		return m_Implementation->IsOpen;
	}
}