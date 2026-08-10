#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	class VulkanMesh;

	class Mesh
	{
	public:
		IGNITION_API ~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		VulkanMesh* GetVulkanMesh() const { return m_VulkanMesh.get(); }

	private:
		friend class Renderer;

		explicit Mesh(std::unique_ptr<VulkanMesh> vulkanMesh);

	private:
		std::unique_ptr<VulkanMesh> m_VulkanMesh;
	};
}