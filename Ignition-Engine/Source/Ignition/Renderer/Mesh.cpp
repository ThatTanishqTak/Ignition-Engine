#include "Ignition/Renderer/Mesh.h"

#include "Ignition/Renderer/MeshImplementation.h"

namespace Ignition
{
	Mesh::Mesh() : m_Implementation(std::make_unique<MeshImplementation>())
	{

	}

	Mesh::~Mesh() = default;

	MeshBounds Mesh::GetBounds() const
	{
		return m_Implementation->Bounds;
	}
}