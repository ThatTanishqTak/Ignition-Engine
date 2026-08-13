#include "Ignition/Renderer/Renderer.h"

#include "Ignition/Renderer/Camera.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Texture.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Renderer/MeshImplementation.h"
#include "Ignition/Renderer/TextureImplementation.h"
#include "Ignition/Renderer/RendererImplementation.h"

namespace Ignition
{
	Renderer::Renderer() : m_Implementation(std::make_unique<RendererImplementation>())
	{

	}

	Renderer::~Renderer() = default;

	bool Renderer::IsValid() const
	{
		return m_Implementation->Backend && m_Implementation->Backend->IsValid();
	}

	void Renderer::SetClearColor(float r, float g, float b, float a)
	{
		if (m_Implementation->Backend)
		{
			m_Implementation->Backend->SetClearColor(r, g, b, a);
		}
	}

	std::shared_ptr<Mesh> Renderer::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (!m_Implementation->Backend)
		{
			return nullptr;
		}

		auto vulkanMesh = m_Implementation->Backend->CreateMesh(vertices, indices);

		if (!vulkanMesh)
		{
			return nullptr;
		}

		auto mesh = std::shared_ptr<Mesh>(new Mesh());
		mesh->m_Implementation->Handle = std::move(vulkanMesh);
		mesh->m_Implementation->Backend = m_Implementation->Backend->GetSelfReference();

		return mesh;
	}

	std::shared_ptr<Texture> Renderer::CreateTexture(const std::string& filepath)
	{
		if (!m_Implementation->Backend)
		{
			return nullptr;
		}

		auto vulkanTexture = m_Implementation->Backend->CreateTexture(filepath);

		if (!vulkanTexture)
		{
			return nullptr;
		}

		auto texture = std::shared_ptr<Texture>(new Texture());
		texture->m_Implementation->Handle = std::move(vulkanTexture);
		texture->m_Implementation->Backend = m_Implementation->Backend->GetSelfReference();

		return texture;
	}

	std::shared_ptr<Texture> Renderer::CreateTextureFromMemory(const void* data, size_t size)
	{
		if (!m_Implementation->Backend)
		{
			return nullptr;
		}

		auto vulkanTexture = m_Implementation->Backend->CreateTextureFromMemory(data, size);

		if (!vulkanTexture)
		{
			return nullptr;
		}

		auto texture = std::shared_ptr<Texture>(new Texture());
		texture->m_Implementation->Handle = std::move(vulkanTexture);
		texture->m_Implementation->Backend = m_Implementation->Backend->GetSelfReference();

		return texture;
	}

	void Renderer::BeginScene(const Camera& camera)
	{
		if (m_Implementation->Backend)
		{
			m_Implementation->Backend->BeginScene(camera.GetViewProjection());
		}
	}

	void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
	{
		if (m_Implementation->Backend && mesh && mesh->m_Implementation->Handle)
		{
			m_Implementation->Backend->Submit(*mesh->m_Implementation->Handle, transform);
		}
	}

	void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const Material& material, const glm::mat4& transform)
	{
		if (m_Implementation->Backend && mesh && mesh->m_Implementation->Handle)
		{
			const VulkanTexture* texture = material.Albedo ? material.Albedo->m_Implementation->Handle.get() : nullptr;

			m_Implementation->Backend->Submit(*mesh->m_Implementation->Handle, texture, material.Tint, material.TwoSided, transform);
		}
	}

	void Renderer::EndScene()
	{
		if (m_Implementation->Backend)
		{
			m_Implementation->Backend->EndScene();
		}
	}

	bool Renderer::IsImGuiFrameActive() const
	{
		return m_Implementation->Backend && m_Implementation->Backend->IsImGuiFrameActive();
	}

	bool Renderer::WantCaptureMouse() const
	{
		return m_Implementation->Backend && m_Implementation->Backend->WantCaptureMouse();
	}

	bool Renderer::WantCaptureKeyboard() const
	{
		return m_Implementation->Backend && m_Implementation->Backend->WantCaptureKeyboard();
	}
}