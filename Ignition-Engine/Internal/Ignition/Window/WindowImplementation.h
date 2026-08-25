#pragma once

#include "Ignition/Input/CursorMode.h"
#include "Ignition/Input/CursorShape.h"

#include <array>
#include <functional>

struct SDL_Window;
struct SDL_Cursor;

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
		CursorShape Shape = CursorShape::Arrow;
		std::array<SDL_Cursor*, CursorShapeCount> SystemCursors{};
		RawEventCallback RawCallback;

		void Initialize(const char* title, int width, int height);
		void Shutdown();
		void PollEvents(EventQueue& eventQueue);
	};
}