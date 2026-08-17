#include "Ignition/Renderer/Renderer.h"

#include "Ignition/Renderer/Camera.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Renderer/Texture.h"
#include "Ignition/Renderer/Material.h"
#include "Ignition/Renderer/MeshImplementation.h"
#include "Ignition/Renderer/TextureImplementation.h"
#include "Ignition/Renderer/RendererImplementation.h"

#include <glm/common.hpp>

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

	template <typename TResource, typename TBackendHandle>
	std::shared_ptr<TResource> Renderer::WrapBackendResource(std::unique_ptr<TBackendHandle> handle)
	{
		if (!handle)
		{
			return nullptr;
		}

		auto resource = std::shared_ptr<TResource>(new TResource());
		resource->m_Implementation->Handle = std::move(handle);
		resource->m_Implementation->Backend = m_Implementation->Backend->GetSelfReference();

		return resource;
	}

	std::shared_ptr<Mesh> Renderer::CreateMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
	{
		if (!m_Implementation->Backend)
		{
			return nullptr;
		}

		auto mesh = WrapBackendResource<Mesh>(m_Implementation->Backend->CreateMesh(vertices, indices));

		if (mesh && !vertices.empty())
		{
			MeshBounds bounds{ vertices.front().Position, vertices.front().Position };

			MeshGeometry geometry;
			geometry.Positions.reserve(vertices.size());
			geometry.Indices = indices;

			for (const Vertex& vertex : vertices)
			{
				bounds.Minimum = glm::min(bounds.Minimum, vertex.Position);
				bounds.Maximum = glm::max(bounds.Maximum, vertex.Position);

				geometry.Positions.push_back(vertex.Position);
			}

			mesh->m_Implementation->Bounds = bounds;
			mesh->m_Implementation->Geometry = std::move(geometry);
		}

		return mesh;
	}

	std::shared_ptr<Texture> Renderer::CreateTexture(const std::string& filepath)
	{
		if (!m_Implementation->Backend)
		{
			return nullptr;
		}

		return WrapBackendResource<Texture>(m_Implementation->Backend->CreateTexture(filepath));
	}

	std::shared_ptr<Texture> Renderer::CreateTextureFromMemory(const void* data, size_t size)
	{
		if (!m_Implementation->Backend)
		{
			return nullptr;
		}

		return WrapBackendResource<Texture>(m_Implementation->Backend->CreateTextureFromMemory(data, size));
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

	void Renderer::SetSceneRenderTargetSize(uint32_t width, uint32_t height)
	{
		if (m_Implementation->Backend)
		{
			m_Implementation->Backend->SetSceneRenderTargetSize(width, height);
		}
	}

	uint64_t Renderer::GetSceneRenderTargetTextureID() const
	{
		return m_Implementation->Backend ? m_Implementation->Backend->GetSceneRenderTargetTextureID() : 0;
	}

	uint32_t Renderer::GetSceneRenderTargetWidth() const
	{
		return m_Implementation->Backend ? m_Implementation->Backend->GetSceneRenderTargetWidth() : 0;
	}

	uint32_t Renderer::GetSceneRenderTargetHeight() const
	{
		return m_Implementation->Backend ? m_Implementation->Backend->GetSceneRenderTargetHeight() : 0;
	}

	const std::vector<PassTiming>& Renderer::GetPassTimings() const
	{
		static const std::vector<PassTiming> empty;

		return m_Implementation->Backend ? m_Implementation->Backend->GetPassTimings() : empty;
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