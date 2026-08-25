#include "Ignition/Scene/Scene.h"

#include "Ignition/Physics/PhysicsComponents.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/SceneRegistry.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/Camera.h"
#include "Ignition/Core/ProfilerInternal.h"

#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/gtc/quaternion.hpp>

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace Ignition
{
	namespace
	{
		// A hand-edited scene file can describe a cycle the API refuses to build, so every walk up the tree is bounded
		constexpr uint32_t MaximumHierarchyDepth = 32;
		constexpr uint32_t NullEntity = 0xFFFFFFFF;

		template<typename TComponent>
		void CopyComponent(Entity source, Entity destination, entt::registry& registry)
		{
			if (const TComponent* component = registry.try_get<TComponent>(static_cast<entt::entity>(source.GetID())))
			{
				registry.emplace_or_replace<TComponent>(static_cast<entt::entity>(destination.GetID()), *component);
			}
		}

		void DecomposeMatrix(const glm::mat4& matrix, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
		{
			position = glm::vec3(matrix[3]);

			glm::vec3 basis[3] = { glm::vec3(matrix[0]), glm::vec3(matrix[1]), glm::vec3(matrix[2]) };
			scale = { glm::length(basis[0]), glm::length(basis[1]), glm::length(basis[2]) };

			for (int axis = 0; axis < 3; ++axis)
			{
				basis[axis] = scale[axis] > 1e-6f ? basis[axis] / scale[axis] : glm::vec3(axis == 0 ? 1.0f : 0.0f, axis == 1 ? 1.0f : 0.0f, axis == 2 ? 1.0f : 0.0f);
			}

			rotation = glm::normalize(glm::quat_cast(glm::mat3(basis[0], basis[1], basis[2])));
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

		// A duplicate lands beside its original rather than at the root, which is what duplicating a wheel means
		CopyComponent<ParentComponent>(entity, duplicate, m_Registry->Registry);

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
		if (entity.m_Scene != this || !entity.IsValid())
		{
			return;
		}

		std::vector<Entity> pending{ entity };
		std::vector<Entity> subtree;
		std::unordered_set<uint32_t> visited;

		while (!pending.empty())
		{
			const Entity current = pending.back();
			pending.pop_back();

			if (!current.IsValid() || !visited.insert(current.GetID()).second)
			{
				continue;
			}

			subtree.push_back(current);

			for (Entity child : GetChildren(current))
			{
				pending.push_back(child);
			}
		}

		for (Entity doomed : subtree)
		{
			m_Registry->Registry.destroy(static_cast<entt::entity>(doomed.GetID()));
		}
	}

	Entity Scene::GetEntity(uint32_t id)
	{
		if (id == NullEntity || !m_Registry->Registry.valid(static_cast<entt::entity>(id)))
		{
			return {};
		}

		return Entity(id, this);
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

	std::vector<Entity> Scene::GetRootEntities()
	{
		std::vector<Entity> roots;

		for (Entity entity : GetEntities())
		{
			if (!GetParent(entity).IsValid())
			{
				roots.push_back(entity);
			}
		}

		return roots;
	}

	std::vector<Entity> Scene::GetChildren(Entity entity)
	{
		std::vector<Entity> children;

		if (entity.m_Scene != this || !entity.IsValid())
		{
			return children;
		}

		const uint32_t parentID = entity.GetID();

		for (const entt::entity handle : m_Registry->Registry.view<ParentComponent>())
		{
			if (m_Registry->Registry.get<ParentComponent>(handle).Parent == parentID)
			{
				children.push_back(Entity(static_cast<uint32_t>(handle), this));
			}
		}

		return children;
	}

	Entity Scene::GetParent(Entity entity)
	{
		if (entity.m_Scene != this || !entity.IsValid())
		{
			return {};
		}

		const ParentComponent* parent = m_Registry->Registry.try_get<ParentComponent>(static_cast<entt::entity>(entity.GetID()));

		return parent ? GetEntity(parent->Parent) : Entity{};
	}

	bool Scene::IsDescendantOf(Entity entity, Entity ancestor)
	{
		if (!entity.IsValid() || !ancestor.IsValid())
		{
			return false;
		}

		Entity walk = GetParent(entity);

		for (uint32_t depth = 0; depth < MaximumHierarchyDepth && walk.IsValid(); ++depth)
		{
			if (walk == ancestor)
			{
				return true;
			}

			walk = GetParent(walk);
		}

		return false;
	}

	bool Scene::SetParent(Entity child, Entity parent, bool keepWorldTransform)
	{
		if (child.m_Scene != this || !child.IsValid())
		{
			return false;
		}

		// Parenting a node to itself or to its own descendant makes a cycle every walk in the engine would then have to survive
		if (parent.IsValid() && (parent == child || IsDescendantOf(parent, child)))
		{
			return false;
		}

		glm::vec3 worldPosition{ 0.0f };
		glm::quat worldRotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 worldScale{ 1.0f };

		if (keepWorldTransform)
		{
			GetWorldTransform(child, worldPosition, worldRotation, worldScale);
		}

		const entt::entity handle = static_cast<entt::entity>(child.GetID());

		if (parent.IsValid())
		{
			m_Registry->Registry.emplace_or_replace<ParentComponent>(handle, ParentComponent{ parent.GetID() });
		}
		else
		{
			m_Registry->Registry.remove<ParentComponent>(handle);
		}

		if (keepWorldTransform)
		{
			SetWorldTransform(child, worldPosition, worldRotation, worldScale);
		}

		return true;
	}

	glm::mat4 Scene::GetParentWorldMatrix(Entity entity)
	{
		const Entity parent = GetParent(entity);

		return parent.IsValid() ? GetWorldMatrix(parent) : glm::mat4(1.0f);
	}

	glm::mat4 Scene::GetWorldMatrix(Entity entity)
	{
		if (entity.m_Scene != this || !entity.IsValid())
		{
			return glm::mat4(1.0f);
		}

		glm::mat4 world = entity.GetTransform().GetMatrix();
		Entity walk = GetParent(entity);

		// Bounded rather than recursive: a cycle from a hand-edited file stops here instead of blowing the stack
		for (uint32_t depth = 0; depth < MaximumHierarchyDepth && walk.IsValid(); ++depth)
		{
			world = walk.GetTransform().GetMatrix() * world;
			walk = GetParent(walk);
		}

		return world;
	}

	void Scene::GetWorldTransform(Entity entity, glm::vec3& position, glm::quat& rotation, glm::vec3& scale)
	{
		if (!GetParent(entity).IsValid())
		{
			// A root's local transform is already its world transform, and taking it directly keeps it exact
			const TransformComponent& transform = entity.GetTransform();

			position = transform.Position;
			rotation = transform.Rotation;
			scale = transform.Scale;

			return;
		}

		DecomposeMatrix(GetWorldMatrix(entity), position, rotation, scale);
	}

	void Scene::SetWorldTransform(Entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale)
	{
		if (entity.m_Scene != this || !entity.IsValid())
		{
			return;
		}

		TransformComponent& transform = entity.GetTransform();

		if (!GetParent(entity).IsValid())
		{
			transform.Position = position;
			transform.Rotation = glm::normalize(rotation);
			transform.Scale = scale;

			return;
		}

		const glm::mat4 world = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(glm::normalize(rotation)) * glm::scale(glm::mat4(1.0f), scale);

		DecomposeMatrix(glm::inverse(GetParentWorldMatrix(entity)) * world, transform.Position, transform.Rotation, transform.Scale);
	}

	void Scene::OnRender(Renderer& renderer, const Camera& camera)
	{
		IG_PROFILE_ZONE();

		renderer.BeginScene(camera);

		const auto view = m_Registry->Registry.view<TransformComponent, MeshRendererComponent>();

		for (const entt::entity handle : view)
		{
			const MeshRendererComponent& meshRenderer = view.get<MeshRendererComponent>(handle);

			renderer.Submit(meshRenderer.Mesh, meshRenderer.Material, GetWorldMatrix(Entity(static_cast<uint32_t>(handle), this)));
		}

		renderer.EndScene();
	}
}