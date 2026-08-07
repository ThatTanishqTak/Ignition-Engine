#pragma once

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

		bool IsValid() const;

		void DrawFrame(float r, float g, float b);
		void OnResize();

	private:
		std::unique_ptr<VulkanRenderer> m_VulkanRenderer;
	};
}