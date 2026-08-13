#include "Ignition/Renderer/Mesh.h"

#include "Ignition/Renderer/MeshImplementation.h"

namespace Ignition
{
	Mesh::Mesh() : m_Implementation(std::make_unique<MeshImplementation>())
	{

	}

	Mesh::~Mesh()
	{
		if (!m_Implementation->Handle)
		{
			return;
		}

		const std::shared_ptr<VulkanRenderer*> renderer = m_Implementation->Backend.lock();

		if (renderer && *renderer)
		{
			(*renderer)->Retire(std::move(m_Implementation->Handle));
		}
		else
		{
			m_Implementation->Handle->Shutdown();
		}
	}
}