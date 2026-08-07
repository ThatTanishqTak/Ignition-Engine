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

	bool Renderer::IsValid() const
	{
		return m_VulkanRenderer && m_VulkanRenderer->IsValid();
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

	void Renderer::SetClearColor(float r, float g, float b, float a)
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->SetClearColor(r, g, b, a);
		}
	}

	void Renderer::BeginFrame()
	{
		if (!m_VulkanRenderer)
		{
			return;
		}
		
		m_VulkanRenderer->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->EndFrame();
		}
	}

	void Renderer::DrawDemoTriangle()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->DrawDemoTriangle();
		}
	}

	void Renderer::OnResize()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->OnResize();
		}
	}
}