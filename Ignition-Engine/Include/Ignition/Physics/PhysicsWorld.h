#pragma once

#include "Ignition/Core/Export.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>

namespace Ignition
{
	class AssetRegistry;
	class Entity;
	class Scene;
	struct PhysicsWorldImplementation;

	struct PhysicsSettings
	{
		glm::vec3 Gravity{ 0.0f, -9.81f, 0.0f };

		// The editor's always-alive query scene never simulates: static shapes only, no dynamics
		bool QueryOnly = false;
	};

	struct RaycastHit
	{
		bool Hit = false;
		uint32_t EntityID = 0xFFFFFFFF;
		glm::vec3 Position{ 0.0f };
		glm::vec3 Normal{ 0.0f };
		float Distance = 0.0f;
	};

	class PhysicsWorld
	{
	public:
		IGNITION_API PhysicsWorld(Scene* scene, AssetRegistry* assets, const PhysicsSettings& settings = {});
		IGNITION_API ~PhysicsWorld();

		PhysicsWorld(const PhysicsWorld&) = delete;
		PhysicsWorld& operator=(const PhysicsWorld&) = delete;

		IGNITION_API bool IsValid() const;

		// Instantiates Jolt bodies from the scene's physics components; safe to call repeatedly
		IGNITION_API void Rebuild();

		IGNITION_API void Step(float fixedTimeStep);

		// Writes interpolated actor poses back onto TransformComponent (alpha = Time::GetFixedAlpha())
		IGNITION_API void SyncTransforms(float alpha);

		// TransformComponent -> body, for gizmo/inspector edits made while playing
		IGNITION_API void PushTransform(Entity entity);

		// Whether this world owns a body for the entity - tells a live body from another world's stale mirror of the same entity
		IGNITION_API bool HasBody(Entity entity) const;

		IGNITION_API RaycastHit Raycast(const glm::vec3& origin, const glm::vec3& direction, float maximumDistance = 1000.0f) const;

		IGNITION_API void SetDebugVisualizationEnabled(bool enabled);
		IGNITION_API bool IsDebugVisualizationEnabled() const;

		// Pushes PhysX's own render buffer through Ignition::DebugDraw
		IGNITION_API void SubmitDebugLines() const;

		IGNITION_API size_t GetBodyCount() const;

		// Drops a sphere onto a plane and reports whether it settled - confirms linkage at startup
		IGNITION_API static bool SelfTest();

	private:
		Scene* m_Scene = nullptr;
		AssetRegistry* m_Assets = nullptr;
		PhysicsSettings m_Settings;
		bool m_DebugVisualization = false;

		std::unique_ptr<PhysicsWorldImplementation> m_Implementation;
	};
}