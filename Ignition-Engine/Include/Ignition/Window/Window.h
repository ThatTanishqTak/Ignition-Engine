#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Input/CursorMode.h"

#include <memory>

namespace Ignition
{
	struct WindowImplementation;

	class Window
	{
	public:
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		IGNITION_API void SetTextInputEnabled(bool enabled);

		IGNITION_API void SetCursorMode(CursorMode mode);
		IGNITION_API CursorMode GetCursorMode() const;

		IGNITION_API void GetPixelSize(int& outWidth, int& outHeight) const;

		IGNITION_API bool IsOpen() const;

	private:
		friend class Engine;

		Window();

		std::unique_ptr<WindowImplementation> m_Implementation;
	};
}