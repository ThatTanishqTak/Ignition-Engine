#pragma once

#include <glm/vec3.hpp>

#include <string>

namespace Ignition
{
	enum class RigidBodyType
	{
		Static = 0,
		Kinematic,
		Dynamic
	};

	struct RigidBodyComponent
	{
		RigidBodyType Type = RigidBodyType::Dynamic;
		float Mass = 1.0f; // kg
		float LinearDamping = 0.01f;
		float AngularDamping = 0.05f;
		bool UseGravity = true;
	};

	// Friction/restitution for every collider on the entity; entities without one get engine defaults
	struct PhysicsMaterialComponent
	{
		float StaticFriction = 0.6f;
		float DynamicFriction = 0.6f;
		float Restitution = 0.0f;
	};

	struct BoxColliderComponent
	{
		glm::vec3 HalfExtents{ 0.5f }; // metres, before the entity's scale
		glm::vec3 Offset{ 0.0f };
	};

	struct SphereColliderComponent
	{
		float Radius = 0.5f;
		glm::vec3 Offset{ 0.0f };
	};

	struct CapsuleColliderComponent
	{
		float Radius = 0.5f;
		float HalfHeight = 0.5f; // half the cylindrical section, caps excluded
		glm::vec3 Offset{ 0.0f };
	};

	struct MeshColliderComponent
	{
		std::string MeshAsset;
		glm::vec3 Offset{ 0.0f };

		// PhysX requires convex hulls for dynamic bodies; triangle meshes are static-only
		bool Convex = true;
	};
}