#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

struct SDL_Window;

namespace Ignition
{
	class VulkanRenderer;

	class Renderer
	{
	public:
		Renderer();
		~Renderer();

		void Initialize(SDL_Window* window);
		void Shutdown();

		IGNITION_API bool IsValid() const;

		IGNITION_API void SetClearColor(float r, float g, float b, float a = 1.0f);

		void BeginFrame();
		void EndFrame();

		void OnResize();

	private:
		std::unique_ptr<VulkanRenderer> m_VulkanRenderer;
	};
}