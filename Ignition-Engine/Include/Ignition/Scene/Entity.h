#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Physics/PhysicsComponents.h"
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
		IGNITION_API void SetName(const std::string& name);

		IGNITION_API MeshRendererComponent& AddMeshRenderer(std::shared_ptr<Mesh> mesh, const Material& material = {});
		IGNITION_API MeshRendererComponent* GetMeshRenderer() const;

		IGNITION_API RigidBodyComponent& AddRigidBody(const RigidBodyComponent& rigidBody = {});
		IGNITION_API RigidBodyComponent* GetRigidBody() const;
		IGNITION_API void RemoveRigidBody();

		IGNITION_API BoxColliderComponent& AddBoxCollider(const BoxColliderComponent& collider = {});
		IGNITION_API BoxColliderComponent* GetBoxCollider() const;
		IGNITION_API void RemoveBoxCollider();

		IGNITION_API SphereColliderComponent& AddSphereCollider(const SphereColliderComponent& collider = {});
		IGNITION_API SphereColliderComponent* GetSphereCollider() const;
		IGNITION_API void RemoveSphereCollider();

		IGNITION_API CapsuleColliderComponent& AddCapsuleCollider(const CapsuleColliderComponent& collider = {});
		IGNITION_API CapsuleColliderComponent* GetCapsuleCollider() const;
		IGNITION_API void RemoveCapsuleCollider();

		IGNITION_API MeshColliderComponent& AddMeshCollider(const MeshColliderComponent& collider = {});
		IGNITION_API MeshColliderComponent* GetMeshCollider() const;
		IGNITION_API void RemoveMeshCollider();

		IGNITION_API PhysicsMaterialComponent& AddPhysicsMaterial(const PhysicsMaterialComponent& material = {});
		IGNITION_API PhysicsMaterialComponent* GetPhysicsMaterial() const;
		IGNITION_API void RemovePhysicsMaterial();

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