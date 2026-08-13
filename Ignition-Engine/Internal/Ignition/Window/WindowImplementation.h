#pragma once

#include "Ignition/Input/CursorMode.h"

#include <functional>

struct SDL_Window;

namespace Ignition
{
	class EventQueue;

	struct WindowImplementation
	{
		using RawEventCallback = std::function<void(const void*)>;

		SDL_Window* SDLWindow = nullptr;
		bool SDLInitialized = false;
		bool IsOpen = false;
		CursorMode Cursor = CursorMode::Normal;
		RawEventCallback RawCallback;

		void Initialize(const char* title, int width, int height);
		void Shutdown();
		void PollEvents(EventQueue& eventQueue);
	};
}