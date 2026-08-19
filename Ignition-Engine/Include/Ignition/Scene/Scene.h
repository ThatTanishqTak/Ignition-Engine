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
		IGNITION_API void DestroyEntity(Entity entity);

		IGNITION_API std::vector<Entity> GetEntities();

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