#include "Ignition/Window/Window.h"

#include "Ignition/Core/Log.h"
#include "Ignition/Events/EventQueue.h"
#include "Ignition/Events/KeyEvent.h"
#include "Ignition/Events/MouseEvent.h"
#include "Ignition/Events/WindowEvent.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

namespace Ignition
{
	Window::Window() = default;
	Window::~Window() = default;

	Window::Window(Window&& other) noexcept : m_SDLWindow(other.m_SDLWindow), m_IsOpen(other.m_IsOpen)
	{
		other.m_SDLWindow = nullptr;
		other.m_IsOpen = false;
	}

	Window& Window::operator=(Window&& other) noexcept
	{
		if (this != &other)
		{
			Shutdown();

			m_SDLWindow = other.m_SDLWindow;
			m_IsOpen = other.m_IsOpen;

			other.m_SDLWindow = nullptr;
			other.m_IsOpen = false;
		}

		return *this;
	}

	void Window::Initialize(const char* title, int width, int height)
	{
		CORE_TRACE("Initializing Window");

		if (!SDL_Init(SDL_INIT_VIDEO))
		{
			CORE_CRITICAL("Failed to initialize SDL: {}", SDL_GetError());

			return;
		}

		m_SDLWindow = SDL_CreateWindow(title, width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
		if (!m_SDLWindow)
		{
			CORE_CRITICAL("Failed to create SDL window: {}", SDL_GetError());
			SDL_Quit();

			return;
		}

		m_IsOpen = true;

		CORE_TRACE("Window Initialized");
	}

	void Window::Shutdown()
	{
		CORE_TRACE("Shutting Down Window");

		if (m_SDLWindow)
		{
			SDL_DestroyWindow(m_SDLWindow);
			m_SDLWindow = nullptr;

			SDL_Quit();
		}

		m_IsOpen = false;

		CORE_TRACE("Window Shutdown Complete");
	}

	void Window::PollEvents(EventQueue& eventQueue)
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_EVENT_QUIT:
					m_IsOpen = false;
					eventQueue.Push<WindowCloseEvent>();
					break;

				case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
					if (m_SDLWindow && event.window.windowID == SDL_GetWindowID(m_SDLWindow))
					{
						m_IsOpen = false;
						eventQueue.Push<WindowCloseEvent>();
					}
					break;

				case SDL_EVENT_WINDOW_RESIZED:
					eventQueue.Push<WindowResizeEvent>(event.window.data1, event.window.data2);
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

				case SDL_EVENT_WINDOW_RESTORED:
					eventQueue.Push<WindowRestoredEvent>();
					break;

				case SDL_EVENT_KEY_DOWN:
					eventQueue.Push<KeyPressedEvent>(static_cast<KeyCode>(event.key.key), event.key.repeat);
					break;

				case SDL_EVENT_KEY_UP:
					eventQueue.Push<KeyReleasedEvent>(static_cast<KeyCode>(event.key.key));
					break;

				case SDL_EVENT_MOUSE_BUTTON_DOWN:
					eventQueue.Push<MouseButtonPressedEvent>(static_cast<MouseCode>(event.button.button));
					break;

				case SDL_EVENT_MOUSE_BUTTON_UP:
					eventQueue.Push<MouseButtonReleasedEvent>(static_cast<MouseCode>(event.button.button));
					break;

				case SDL_EVENT_MOUSE_MOTION:
					eventQueue.Push<MouseMovedEvent>(event.motion.x, event.motion.y);
					break;

				case SDL_EVENT_MOUSE_WHEEL:
					eventQueue.Push<MouseScrolledEvent>(event.wheel.x, event.wheel.y);
					break;

				default:
					break;
			}
		}
	}

	const char* const* Window::GetRequiredVulkanExtensions(uint32_t* outCount)
	{
		return SDL_Vulkan_GetInstanceExtensions(outCount);
	}
}