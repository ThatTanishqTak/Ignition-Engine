#pragma once

#include "Ignition/Core/Export.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace Ignition
{
	struct MeshImplementation;

	struct MeshBounds
	{
		glm::vec3 Minimum{ 0.0f };
		glm::vec3 Maximum{ 0.0f };
	};

	// CPU-side copy kept for systems that consume geometry rather than draw it (collider cooking, voxelization)
	struct MeshGeometry
	{
		std::vector<glm::vec3> Positions;
		std::vector<uint32_t> Indices;
	};

	class Mesh
	{
	public:
		IGNITION_API ~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		IGNITION_API MeshBounds GetBounds() const;
		IGNITION_API const MeshGeometry& GetGeometry() const;

	private:
		friend class Renderer;

		Mesh();

		std::unique_ptr<MeshImplementation> m_Implementation;
	};
}