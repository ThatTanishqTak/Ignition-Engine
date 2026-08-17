#include "Ignition/Scene/Scene.h"

#include "Ignition/Physics/PhysicsComponents.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/SceneRegistry.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Camera.h"

namespace Ignition
{
	namespace
	{
		template<typename TComponent>
		void CopyComponent(Entity source, Entity destination, entt::registry& registry)
		{
			if (const TComponent* component = registry.try_get<TComponent>(static_cast<entt::entity>(source.GetID())))
			{
				registry.emplace_or_replace<TComponent>(static_cast<entt::entity>(destination.GetID()), *component);
			}
		}
	}

	Scene::Scene() : m_Registry(std::make_unique<SceneRegistry>())
	{

	}

	Scene::~Scene() = default;

	Entity Scene::CreateEntity(const std::string& name)
	{
		const entt::entity handle = m_Registry->Registry.create();

		m_Registry->Registry.emplace<TagComponent>(handle, name);
		m_Registry->Registry.emplace<TransformComponent>(handle);

		return Entity(static_cast<uint32_t>(handle), this);
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		if (entity.m_Scene != this || !entity.IsValid())
		{
			return {};
		}

		Entity duplicate = CreateEntity(entity.GetName() + " Copy");

		duplicate.GetTransform() = entity.GetTransform();

		if (const MeshRendererComponent* meshRenderer = entity.GetMeshRenderer())
		{
			m_Registry->Registry.emplace_or_replace<MeshRendererComponent>(static_cast<entt::entity>(duplicate.GetID()), *meshRenderer);
		}

		CopyComponent<RigidBodyComponent>(entity, duplicate, m_Registry->Registry);
		CopyComponent<PhysicsMaterialComponent>(entity, duplicate, m_Registry->Registry);
		CopyComponent<BoxColliderComponent>(entity, duplicate, m_Registry->Registry);
		CopyComponent<SphereColliderComponent>(entity, duplicate, m_Registry->Registry);
		CopyComponent<CapsuleColliderComponent>(entity, duplicate, m_Registry->Registry);
		CopyComponent<MeshColliderComponent>(entity, duplicate, m_Registry->Registry);

		return duplicate;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		if (entity.m_Scene == this && entity.IsValid())
		{
			m_Registry->Registry.destroy(static_cast<entt::entity>(entity.GetID()));
		}
	}

	std::vector<Entity> Scene::GetEntities()
	{
		const auto view = m_Registry->Registry.view<TagComponent>();

		std::vector<Entity> entities;
		entities.reserve(view.size());

		for (const entt::entity handle : view)
		{
			entities.push_back(Entity(static_cast<uint32_t>(handle), this));
		}

		return entities;
	}

	void Scene::OnRender(Renderer& renderer, const Camera& camera)
	{
		renderer.BeginScene(camera);

		const auto view = m_Registry->Registry.view<TransformComponent, MeshRendererComponent>();

		for (const entt::entity handle : view)
		{
			const auto& [transform, meshRenderer] = view.get<TransformComponent, MeshRendererComponent>(handle);

			renderer.Submit(meshRenderer.Mesh, meshRenderer.Material, transform.GetMatrix());
		}

		renderer.EndScene();
	}
}