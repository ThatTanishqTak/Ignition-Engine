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
}