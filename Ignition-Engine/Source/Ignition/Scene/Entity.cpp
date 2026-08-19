#include "Ignition/Scene/Entity.h"

#include "Ignition/Scene/Scene.h"
#include "Ignition/Scene/SceneRegistry.h"

#include <utility>

namespace Ignition
{
	namespace
	{
		entt::entity ToHandle(uint32_t id)
		{
			return static_cast<entt::entity>(id);
		}
	}

	bool Entity::IsValid() const
	{
		return m_Scene && m_Scene->m_Registry->Registry.valid(ToHandle(m_ID));
	}

	TransformComponent& Entity::GetTransform() const
	{
		return m_Scene->m_Registry->Registry.get<TransformComponent>(ToHandle(m_ID));
	}

	const std::string& Entity::GetName() const
	{
		return m_Scene->m_Registry->Registry.get<TagComponent>(ToHandle(m_ID)).Name;
	}

	void Entity::SetName(const std::string& name)
	{
		m_Scene->m_Registry->Registry.get<TagComponent>(ToHandle(m_ID)).Name = name;
	}

	MeshRendererComponent& Entity::AddMeshRenderer(std::shared_ptr<Mesh> mesh, const Material& material)
	{
		return m_Scene->m_Registry->Registry.emplace_or_replace<MeshRendererComponent>(ToHandle(m_ID), std::move(mesh), material);
	}

	MeshRendererComponent* Entity::GetMeshRenderer() const
	{
		if (!IsValid())
		{
			return nullptr;
		}

		return m_Scene->m_Registry->Registry.try_get<MeshRendererComponent>(ToHandle(m_ID));
	}
	
// Every body expands inside an Entity member - Scene befriends the class, not free helpers
#define IG_DEFINE_COMPONENT_ACCESSORS(Type, Name)                                                 \
	Type& Entity::Add##Name(const Type& component)                                                \
	{                                                                                             \
		return m_Scene->m_Registry->Registry.emplace_or_replace<Type>(ToHandle(m_ID), component); \
	}                                                                                             \
                                                                                                  \
	Type* Entity::Get##Name() const                                                               \
	{                                                                                             \
		return IsValid() ? m_Scene->m_Registry->Registry.try_get<Type>(ToHandle(m_ID)) : nullptr; \
	}                                                                                             \
                                                                                                  \
	void Entity::Remove##Name()                                                                   \
	{                                                                                             \
		if (IsValid())                                                                            \
		{                                                                                         \
			m_Scene->m_Registry->Registry.remove<Type>(ToHandle(m_ID));                           \
		}                                                                                         \
	}

	IG_DEFINE_COMPONENT_ACCESSORS(RigidBodyComponent, RigidBody)
	IG_DEFINE_COMPONENT_ACCESSORS(BoxColliderComponent, BoxCollider)
	IG_DEFINE_COMPONENT_ACCESSORS(SphereColliderComponent, SphereCollider)
	IG_DEFINE_COMPONENT_ACCESSORS(CapsuleColliderComponent, CapsuleCollider)
	IG_DEFINE_COMPONENT_ACCESSORS(MeshColliderComponent, MeshCollider)
	IG_DEFINE_COMPONENT_ACCESSORS(PhysicsMaterialComponent, PhysicsMaterial)
	IG_DEFINE_COMPONENT_ACCESSORS(WindTunnelComponent, WindTunnel)
	IG_DEFINE_COMPONENT_ACCESSORS(AeroBodyComponent, AeroBody)

#undef IG_DEFINE_COMPONENT_ACCESSORS
}