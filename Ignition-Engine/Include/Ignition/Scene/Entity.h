#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Scene/Components.h"

#include <cstdint>
#include <memory>
#include <string>

namespace Ignition
{
	class Scene;

	class Entity
	{
	public:
		Entity() = default;

		IGNITION_API bool IsValid() const;

		IGNITION_API TransformComponent& GetTransform() const;
		IGNITION_API const std::string& GetName() const;

		IGNITION_API MeshRendererComponent& AddMeshRenderer(std::shared_ptr<Mesh> mesh, const Material& material = {});
		IGNITION_API MeshRendererComponent* GetMeshRenderer() const;

		uint32_t GetID() const { return m_ID; }

		bool operator==(const Entity& other) const { return m_ID == other.m_ID && m_Scene == other.m_Scene; }

	private:
		friend class Scene;

		Entity(uint32_t id, Scene* scene) : m_ID(id), m_Scene(scene) {}

	private:
		uint32_t m_ID = 0xFFFFFFFF; // entt::null
		Scene* m_Scene = nullptr;
	};
}