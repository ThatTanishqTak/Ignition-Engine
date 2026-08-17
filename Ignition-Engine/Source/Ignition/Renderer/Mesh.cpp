#include "Ignition/Renderer/Mesh.h"

#include "Ignition/Renderer/MeshImplementation.h"

namespace Ignition
{
	Mesh::Mesh() : m_Implementation(std::make_unique<MeshImplementation>())
	{

	}

	Mesh::~Mesh() = default;

	const MeshGeometry& Mesh::GetGeometry() const
	{
		return m_Implementation->Geometry;
	}

	MeshBounds Mesh::GetBounds() const
	{
		return m_Implementation->Bounds;
	}
}