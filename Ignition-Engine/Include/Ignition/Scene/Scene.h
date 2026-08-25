#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Fluid/FluidTypes.h"
#include "Ignition/Scene/Entity.h"

#include <memory>
#include <string>
#include <vector>

namespace Ignition
{
	class Camera;
	class PhysicsWorld;
	class Renderer;
	class SceneRegistry;
	class SceneSerializer;

	class Scene
	{
	public:
		IGNITION_API Scene();
		IGNITION_API ~Scene();

		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;

		IGNITION_API Entity CreateEntity(const std::string& name = "Entity");
		IGNITION_API Entity DuplicateEntity(Entity entity);

		// Destroys the whole subtree: an orphaned child with a stale parent id is worse than a deleted one
		IGNITION_API void DestroyEntity(Entity entity);

		IGNITION_API Entity GetEntity(uint32_t id);
		IGNITION_API std::vector<Entity> GetEntities();

		// Transform hierarchy. TransformComponent is local to the parent; world space is only ever derived, never stored
		IGNITION_API std::vector<Entity> GetRootEntities();
		IGNITION_API std::vector<Entity> GetChildren(Entity entity);
		IGNITION_API Entity GetParent(Entity entity);
		IGNITION_API bool IsDescendantOf(Entity entity, Entity ancestor);

		// Refuses a cycle and returns false. Keeping the world transform is what a reparent in an editor means
		IGNITION_API bool SetParent(Entity child, Entity parent, bool keepWorldTransform = true);

		IGNITION_API glm::mat4 GetParentWorldMatrix(Entity entity);
		IGNITION_API glm::mat4 GetWorldMatrix(Entity entity);
		IGNITION_API void GetWorldTransform(Entity entity, glm::vec3& position, glm::quat& rotation, glm::vec3& scale);
		IGNITION_API void SetWorldTransform(Entity entity, const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale);

		IGNITION_API void OnRender(Renderer& renderer, const Camera& camera);

		// The whole scene sits in the tunnel, so the tunnel is scene state rather than something an entity carries
		IGNITION_API FluidSolver3DSettings& GetWindTunnel() { return m_WindTunnel; }
		IGNITION_API const FluidSolver3DSettings& GetWindTunnel() const { return m_WindTunnel; }

	private:
		friend class Entity;
		friend class PhysicsWorld;
		friend class SceneSerializer;

		std::unique_ptr<SceneRegistry> m_Registry;
		FluidSolver3DSettings m_WindTunnel;
	};
}