#pragma once

#include "Ignition/Core/Export.h"

#include <glm/vec3.hpp>

#include <memory>

namespace Ignition
{
	struct MeshImplementation;

	struct MeshBounds
	{
		glm::vec3 Minimum{ 0.0f };
		glm::vec3 Maximum{ 0.0f };
	};

	class Mesh
	{
	public:
		IGNITION_API ~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		IGNITION_API MeshBounds GetBounds() const;

	private:
		friend class Renderer;

		Mesh();

		std::unique_ptr<MeshImplementation> m_Implementation;
	};
}