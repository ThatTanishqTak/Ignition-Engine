#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Input/CursorMode.h"

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

		void SetTextInputEnabled(bool enabled);

		void SetCursorMode(CursorMode mode);
		CursorMode GetCursorMode() const { return m_CursorMode; }

		bool IsOpen() const { return m_IsOpen; }
		SDL_Window* GetNativeWindow() const { return m_SDLWindow; }

	private:
		SDL_Window* m_SDLWindow = nullptr;
		bool m_SDLInitialized = false;
		bool m_IsOpen = false;
		CursorMode m_CursorMode = CursorMode::Normal;
	};
}