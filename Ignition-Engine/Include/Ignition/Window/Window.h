#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Input/CursorMode.h"

#include <cstdint>
#include <functional>

struct SDL_Window;

namespace Ignition
{
	class EventQueue;

	class Window
	{
	public:
		Window();
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		Window(Window&& other) noexcept;
		Window& operator=(Window&& other) noexcept;

		IGNITION_API void Initialize(const char* title, int width, int height);
		IGNITION_API void Shutdown();

		IGNITION_API void PollEvents(EventQueue& eventQueue);
		
		using RawEventCallback = std::function<void(const void*)>;
		IGNITION_API void SetRawEventCallback(RawEventCallback callback) { m_RawEventCallback = std::move(callback); }

		IGNITION_API void SetTextInputEnabled(bool enabled);

		IGNITION_API void SetCursorMode(CursorMode mode);
		IGNITION_API CursorMode GetCursorMode() const { return m_CursorMode; }

		IGNITION_API void GetPixelSize(int& outWidth, int& outHeight) const;

		IGNITION_API bool IsOpen() const { return m_IsOpen; }
		IGNITION_API SDL_Window* GetNativeWindow() const { return m_SDLWindow; }

	private:
		SDL_Window* m_SDLWindow = nullptr;
		bool m_SDLInitialized = false;
		bool m_IsOpen = false;
		CursorMode m_CursorMode = CursorMode::Normal;
		RawEventCallback m_RawEventCallback;
	};
}