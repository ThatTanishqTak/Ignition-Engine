#include "Ignition/Window/Window.h"

#include "Ignition/Window/WindowImplementation.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/EventQueue.h"
#include "Ignition/Events/GamepadEvent.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"

#include <SDL3/SDL.h>

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
					eventQueue.Push<KeyPressedEvent>(static_cast<KeyCode>(event.key.key), static_cast<ScanCode>(event.key.scancode), event.key.repeat);
					break;

				case SDL_EVENT_KEY_UP:
					eventQueue.Push<KeyReleasedEvent>(static_cast<KeyCode>(event.key.key), static_cast<ScanCode>(event.key.scancode));
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					eventQueue.Push<MouseButtonPressedEvent>(static_cast<MouseCode>(event.button.button));
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP:
					eventQueue.Push<MouseButtonReleasedEvent>(static_cast<MouseCode>(event.button.button));
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

	bool Window::IsOpen() const
	{
		return m_Implementation->IsOpen;
	}
}