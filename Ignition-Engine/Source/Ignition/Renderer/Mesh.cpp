#include "Ignition/Renderer/Mesh.h"

#include "Ignition/Renderer/Vulkan/VulkanMesh.h"

namespace Ignition
{
	Mesh::Mesh(std::unique_ptr<VulkanMesh> vulkanMesh) : m_VulkanMesh(std::move(vulkanMesh))
	{

	}

	Mesh::~Mesh()
	{
		if (m_VulkanMesh)
		{
			m_VulkanMesh->Shutdown();
		}
	}
}