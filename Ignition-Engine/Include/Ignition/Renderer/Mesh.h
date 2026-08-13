#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	struct MeshImplementation;

	class Mesh
	{
	public:
		IGNITION_API ~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

	private:
		friend class Renderer;

		Mesh();

		std::unique_ptr<MeshImplementation> m_Implementation;
	};
}