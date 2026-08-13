#pragma once

#include "Ignition/Core/Export.h"

#include <memory>

namespace Ignition
{
	class VulkanMesh;
	class VulkanRenderer;

	class Mesh
	{
	public:
		IGNITION_API ~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;

		VulkanMesh* GetVulkanMesh() const { return m_VulkanMesh.get(); }

	private:
		friend class Renderer;

		Mesh(std::unique_ptr<VulkanMesh> vulkanMesh, std::weak_ptr<VulkanRenderer*> renderer);

	private:
		std::unique_ptr<VulkanMesh> m_VulkanMesh;
		std::weak_ptr<VulkanRenderer*> m_Renderer;
	};
}