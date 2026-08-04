#pragma once

#include "Ignition/Core/Export.h"

#include <cstdint>

struct SDL_Window;

namespace Ignition
{
	class EventQueue;

	class IGNITION_API Window
	{
	public:
		Window();
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		Window(Window&& other) noexcept;
		Window& operator=(Window&& other) noexcept;

		void Initialize(const char* title, int width, int height);
		void Shutdown();

		void PollEvents(EventQueue& eventQueue);

		bool IsOpen() const { return m_IsOpen; }
		SDL_Window* GetNativeWindow() const { return m_SDLWindow; }

		static const char* const* GetRequiredVulkanExtensions(uint32_t* outCount);

	private:
		SDL_Window* m_SDLWindow = nullptr;
		bool m_IsOpen = false;
	};
}