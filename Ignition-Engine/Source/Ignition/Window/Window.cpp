#include "Ignition/Window/Window.h"

#include "Ignition/Core/Log.h"

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

	void Window::PollEvents()
	{
		SDL_Event event;

		while (SDL_PollEvent(&event))
		{
			switch (event.type)
			{
			case SDL_EVENT_QUIT:
				m_IsOpen = false;
				break;

			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				if (m_SDLWindow && event.window.windowID == SDL_GetWindowID(m_SDLWindow))
				{
					m_IsOpen = false;
				}
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