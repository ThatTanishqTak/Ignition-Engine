#include "Ignition/Renderer/Renderer.h"

#include "Ignition/Renderer/Camera.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Texture.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"
#include "Ignition/Renderer/Vulkan/VulkanMesh.h"
#include "Ignition/Renderer/Vulkan/VulkanTexture.h"

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

	std::shared_ptr<Mesh> Renderer::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (!m_VulkanRenderer)
		{
			return nullptr;
		}

		auto vulkanMesh = m_VulkanRenderer->CreateMesh(vertices, indices);

		if (!vulkanMesh)
		{
			return nullptr;
		}

		return std::shared_ptr<Mesh>(new Mesh(std::move(vulkanMesh), m_VulkanRenderer->GetSelfReference()));
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->BeginScene(camera.GetViewProjection());
		}
	}

	std::shared_ptr<Texture> Renderer::CreateTexture(const std::string& filepath)
	{
		if (!m_VulkanRenderer)
		{
			return nullptr;
		}

		auto vulkanTexture = m_VulkanRenderer->CreateTexture(filepath);

		if (!vulkanTexture)
		{
			return nullptr;
		}

		return std::shared_ptr<Texture>(new Texture(std::move(vulkanTexture), m_VulkanRenderer->GetSelfReference()));
	}

	void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
	{
		if (m_VulkanRenderer && mesh && mesh->GetVulkanMesh())
		{
			m_VulkanRenderer->Submit(*mesh->GetVulkanMesh(), transform);
		}
	}

	void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const Material& material, const glm::mat4& transform)
	{
		if (m_VulkanRenderer && mesh && mesh->GetVulkanMesh())
		{
			const VulkanTexture* texture = material.Albedo ? material.Albedo->GetVulkanTexture() : nullptr;

			m_VulkanRenderer->Submit(*mesh->GetVulkanMesh(), texture, material.Tint, transform);
		}
	}

	void Renderer::EndScene()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->EndScene();
		}
	}

	void Renderer::ProcessImGuiEvent(const void* sdlEvent)
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->ProcessImGuiEvent(sdlEvent);
		}
	}

	void Renderer::BeginImGuiFrame()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->BeginImGuiFrame();
		}
	}

	bool Renderer::IsImGuiFrameActive() const
	{
		return m_VulkanRenderer && m_VulkanRenderer->IsImGuiFrameActive();
	}

	bool Renderer::WantCaptureMouse() const
	{
		return m_VulkanRenderer && m_VulkanRenderer->WantCaptureMouse();
	}

	bool Renderer::WantCaptureKeyboard() const
	{
		return m_VulkanRenderer && m_VulkanRenderer->WantCaptureKeyboard();
	}

	void Renderer::WaitIdle()
	{
		if (m_VulkanRenderer)
		{
			m_VulkanRenderer->WaitIdle();
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