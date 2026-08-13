#include "Ignition/Renderer/Mesh.h"

#include "Ignition/Renderer/Vulkan/VulkanMesh.h"
#include "Ignition/Renderer/Vulkan/VulkanRenderer.h"

namespace Ignition
{
	Mesh::Mesh(std::unique_ptr<VulkanMesh> vulkanMesh, std::weak_ptr<VulkanRenderer*> renderer) : m_VulkanMesh(std::move(vulkanMesh)), m_Renderer(std::move(renderer))
	{

	}

	Mesh::~Mesh()
	{
		if (!m_VulkanMesh)
		{
			return;
		}

		const std::shared_ptr<VulkanRenderer*> renderer = m_Renderer.lock();

		if (renderer && *renderer)
		{
			(*renderer)->Retire(std::move(m_VulkanMesh));
		}
		else
		{
			m_VulkanMesh->Shutdown();
		}
	}
}