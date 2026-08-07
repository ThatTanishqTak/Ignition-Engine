#include "Ignition/Renderer/Renderer.h"

#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

#include "Ignition/Core/Log.h"

namespace Ignition
{
	Renderer::Renderer() = default;
	Renderer::~Renderer() = default;

	void Renderer::Initialize(SDL_Window* window)
	{
		IG_CORE_INFO("------- INITIALIZING RENDERER -------");

		m_VulkanRenderer = std::make_unique<VulkanRenderer>();
		m_VulkanRenderer->Initialize(window);

		IG_CORE_INFO("------- RENDERER INITIALIZED -------");
	}

	void Renderer::Shutdown()
	{
		IG_CORE_INFO("------- SHUTTING DOWN RENDERER -------");

		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->Shutdown();
			m_VulkanRenderer.reset();
		}

		IG_CORE_INFO("------- RENDERER SHUTDOWN COMPLETE -------");
	}

	void Renderer::DrawFrame(float r, float g, float b)
	{
		if (!m_VulkanRenderer)
		{
			return;
		}

		m_VulkanRenderer->SetClearColor(r, g, b);
		m_VulkanRenderer->BeginFrame();

		m_VulkanRenderer->EndFrame();
	}

	void Renderer::OnResize()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->OnResize();
		}
	}
}