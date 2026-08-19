#include "Ignition/Physics/PhysicsWorld.h"

#include "Ignition/Core/ProfilerInternal.h"

#include "Ignition/Assets/AssetRegistry.h"
#include "Ignition/Core/Log.h"
#include "Ignition/Physics/PhysicsComponents.h"
#include "Ignition/Physics/PhysicsWorldImplementation.h"
#include "Ignition/Renderer/DebugDraw.h"
#include "Ignition/Renderer/Mesh.h"
#include "Ignition/Scene/Components.h"
#include "Ignition/Scene/Entity.h"
#include "Ignition/Scene/Scene.h"
#include "Ignition/Scene/SceneRegistry.h"

#include <Jolt/Core/Factory.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/RegisterTypes.h>

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#endif

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>

#include <algorithm>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <thread>
#include <vector>

namespace Ignition
{
	namespace
	{
		constexpr JPH::uint MaximumBodies = 8192;
		constexpr JPH::uint BodyMutexCount = 0; // 0 = Jolt picks a default
		constexpr JPH::uint MaximumBodyPairs = 8192;
		constexpr JPH::uint MaximumContactConstraints = 4096;

		PhysicsContext g_PhysicsContext;

		JPH::Vec3 ToJolt(const glm::vec3& value)
		{
			return JPH::Vec3(value.x, value.y, value.z);
		}

		glm::vec3 FromJolt(const JPH::Vec3& value)
		{
			return glm::vec3(value.GetX(), value.GetY(), value.GetZ());
		}

		JPH::Quat ToJolt(const glm::quat& value)
		{
			return JPH::Quat(value.x, value.y, value.z, value.w);
		}

		glm::quat FromJolt(const JPH::Quat& value)
		{
			return glm::quat(value.GetW(), value.GetX(), value.GetY(), value.GetZ());
		}

		JPH::Quat ToJoltRotation(const glm::quat& rotation)
		{
			return ToJolt(glm::normalize(rotation));
		}

		// Jolt shapes cannot carry non-uniform scale directly, so it is baked into the geometry
		float UniformScale(const glm::vec3& scale)
		{
			return glm::max(glm::max(std::abs(scale.x), std::abs(scale.y)), std::abs(scale.z));
		}

		void JoltTrace(const char* format, ...)
		{
			char buffer[1024];

			va_list arguments;
			va_start(arguments, format);
			std::vsnprintf(buffer, sizeof(buffer), format, arguments);
			va_end(arguments);

			IG_CORE_TRACE("Jolt: {}", buffer);
		}

#ifdef JPH_ENABLE_ASSERTS
		bool JoltAssertFailed(const char* expression, const char* message, const char* file, JPH::uint line)
		{
			IG_CORE_ERROR("Jolt assert at {}:{} - ({}) {}", file, line, expression, message ? message : "");

			return true;
		}
#endif

		JPH::RefConst<JPH::Shape> FinalizeShape(const JPH::ShapeSettings::ShapeResult& result, const char* what)
		{
			if (result.HasError())
			{
				IG_CORE_ERROR("Jolt shape creation failed ({}): {}", what, result.GetError().c_str());

				return nullptr;
			}

			return result.Get();
		}

		JPH::RefConst<JPH::Shape> ApplyOffset(const JPH::RefConst<JPH::Shape>& shape, const glm::vec3& offset)
		{
			if (!shape || glm::length(offset) <= 0.0f)
			{
				return shape;
			}

			const JPH::RotatedTranslatedShapeSettings settings(ToJolt(offset), JPH::Quat::sIdentity(), shape);

			return FinalizeShape(settings.Create(), "offset");
		}

		struct ColliderShape
		{
			JPH::RefConst<JPH::Shape> Shape;
			glm::vec3 Offset{ 0.0f };
		};

		JPH::RefConst<JPH::Shape> CombineShapes(const std::vector<ColliderShape>& shapes)
		{
			if (shapes.empty())
			{
				return nullptr;
			}

			if (shapes.size() == 1)
			{
				return ApplyOffset(shapes.front().Shape, shapes.front().Offset);
			}

			JPH::StaticCompoundShapeSettings settings;

			for (const ColliderShape& entry : shapes)
			{
				settings.AddShape(ToJolt(entry.Offset), JPH::Quat::sIdentity(), entry.Shape);
			}

			return FinalizeShape(settings.Create(), "compound");
		}

		JPH::RefConst<JPH::Shape> BuildMeshShape(PhysicsWorldImplementation& implementation, AssetRegistry& assets, const std::string& meshAsset, bool convex, const glm::vec3& scale)
		{
			auto& cache = convex ? implementation.ConvexShapes : implementation.TriangleShapes;

			if (const auto cached = cache.find(meshAsset); cached != cache.end())
			{
				return cached->second ? FinalizeShape(JPH::ScaledShapeSettings(cached->second, ToJolt(scale)).Create(), "scaled mesh") : nullptr;
			}

			const std::shared_ptr<Mesh> mesh = assets.LoadMesh(meshAsset);

			if (!mesh)
			{
				IG_CORE_ERROR("Mesh collider references '{}', which failed to load", meshAsset);

				return nullptr;
			}

			const MeshGeometry& geometry = mesh->GetGeometry();

			if (geometry.Positions.empty() || geometry.Indices.size() < 3)
			{
				return nullptr;
			}

			JPH::RefConst<JPH::Shape> shape;

			if (convex)
			{
				JPH::Array<JPH::Vec3> points;
				points.reserve(geometry.Positions.size());

				for (const glm::vec3& position : geometry.Positions)
				{
					points.push_back(ToJolt(position));
				}

				shape = FinalizeShape(JPH::ConvexHullShapeSettings(points).Create(), "convex hull");
			}
			else
			{
				JPH::VertexList vertices;
				vertices.reserve(geometry.Positions.size());

				for (const glm::vec3& position : geometry.Positions)
				{
					vertices.push_back(JPH::Float3(position.x, position.y, position.z));
				}

				JPH::IndexedTriangleList triangles;
				triangles.reserve(geometry.Indices.size() / 3);

				for (size_t index = 0; index + 2 < geometry.Indices.size(); index += 3)
				{
					triangles.push_back(JPH::IndexedTriangle(geometry.Indices[index], geometry.Indices[index + 1], geometry.Indices[index + 2], 0));
				}

				JPH::MeshShapeSettings settings(std::move(vertices), std::move(triangles));
				settings.Sanitize(); // drops degenerate triangles rather than asserting on them

				shape = FinalizeShape(settings.Create(), "triangle mesh");
			}

			cache.emplace(meshAsset, shape);

			if (!shape)
			{
				return nullptr;
			}

			return FinalizeShape(JPH::ScaledShapeSettings(shape, ToJolt(scale)).Create(), "scaled mesh");
		}

#ifdef JPH_DEBUG_RENDERER
		class JoltDebugRenderer final : public JPH::DebugRendererSimple
		{
		public:
			void DrawLine(JPH::RVec3Arg from, JPH::RVec3Arg to, JPH::ColorArg color) override
			{
				const glm::vec4 lineColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, 1.0f);

				DebugDraw::Line(FromJolt(JPH::Vec3(from)), FromJolt(JPH::Vec3(to)), lineColor);
			}

			void DrawText3D(JPH::RVec3Arg, const std::string_view&, JPH::ColorArg, float) override
			{
				// Text has no place in a line renderer
			}
		};
#endif
	}

	PhysicsContext* PhysicsContext::Acquire()
	{
		if (g_PhysicsContext.m_ReferenceCount == 0)
		{
			JPH::RegisterDefaultAllocator();

			JPH::Trace = JoltTrace;
			JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = JoltAssertFailed;)

				JPH::Factory::sInstance = new JPH::Factory();
			JPH::RegisterTypes();

			IG_CORE_INFO("Jolt Physics initialized (CPU)");
		}

		++g_PhysicsContext.m_ReferenceCount;

		return &g_PhysicsContext;
	}

	void PhysicsContext::Release()
	{
		if (g_PhysicsContext.m_ReferenceCount == 0 || --g_PhysicsContext.m_ReferenceCount > 0)
		{
			return;
		}

		JPH::UnregisterTypes();

		delete JPH::Factory::sInstance;
		JPH::Factory::sInstance = nullptr;
	}

	PhysicsWorld::PhysicsWorld(Scene* scene, AssetRegistry* assets, const PhysicsSettings& settings) : m_Scene(scene), m_Assets(assets), m_Settings(settings), m_Implementation(std::make_unique<PhysicsWorldImplementation>())
	{
		m_Implementation->Context = PhysicsContext::Acquire();

		m_Implementation->System.Init(MaximumBodies, BodyMutexCount, MaximumBodyPairs, MaximumContactConstraints,
			m_Implementation->BroadPhaseLayerInterface,
			m_Implementation->ObjectVsBroadPhaseLayerFilter,
			m_Implementation->ObjectLayerPairFilter);

		m_Implementation->System.SetGravity(ToJolt(m_Settings.Gravity));

		m_Implementation->TempAllocator = std::make_unique<JPH::TempAllocatorImpl>(16 * 1024 * 1024);

		const unsigned int hardwareThreads = std::thread::hardware_concurrency();
		const int workerThreads = m_Settings.QueryOnly ? 1 : static_cast<int>(std::max(1u, hardwareThreads > 1 ? hardwareThreads - 1 : 1u));

		m_Implementation->JobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, workerThreads);

		m_Implementation->Initialized = true;

		Rebuild();
	}

	PhysicsWorld::~PhysicsWorld()
	{
		if (m_Implementation->Initialized)
		{
			JPH::BodyInterface& bodyInterface = m_Implementation->System.GetBodyInterface();

			for (const auto& [entityID, body] : m_Implementation->Bodies)
			{
				if (!body.ID.IsInvalid())
				{
					bodyInterface.RemoveBody(body.ID);
					bodyInterface.DestroyBody(body.ID);
				}
			}
		}

		m_Implementation->Bodies.clear();
		m_Implementation->ConvexShapes.clear();
		m_Implementation->TriangleShapes.clear();

		m_Implementation->JobSystem.reset();
		m_Implementation->TempAllocator.reset();

		if (m_Implementation->Context)
		{
			PhysicsContext::Release();
			m_Implementation->Context = nullptr;
		}
	}

	bool PhysicsWorld::IsValid() const
	{
		return m_Implementation && m_Implementation->Initialized;
	}

	void PhysicsWorld::Rebuild()
	{
		IG_PROFILE_ZONE();

		if (!IsValid() || !m_Scene)
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = m_Implementation->System.GetBodyInterface();

		for (const auto& [entityID, body] : m_Implementation->Bodies)
		{
			if (!body.ID.IsInvalid())
			{
				bodyInterface.RemoveBody(body.ID);
				bodyInterface.DestroyBody(body.ID);
			}
		}

		m_Implementation->Bodies.clear();

		entt::registry& registry = m_Scene->m_Registry->Registry;

		for (const entt::entity handle : registry.view<TransformComponent>())
		{
			const TransformComponent& transform = registry.get<TransformComponent>(handle);

			const RigidBodyComponent* rigidBody = registry.try_get<RigidBodyComponent>(handle);
			const BoxColliderComponent* boxCollider = registry.try_get<BoxColliderComponent>(handle);
			const SphereColliderComponent* sphereCollider = registry.try_get<SphereColliderComponent>(handle);
			const CapsuleColliderComponent* capsuleCollider = registry.try_get<CapsuleColliderComponent>(handle);
			const MeshColliderComponent* meshCollider = registry.try_get<MeshColliderComponent>(handle);
			const MeshRendererComponent* meshRenderer = registry.try_get<MeshRendererComponent>(handle);

			const bool hasCollider = boxCollider || sphereCollider || capsuleCollider || meshCollider;

			// Wind-tunnel geometry needs no physics, but a mesh the mouse cannot select is a bug in an editor. The query world falls back to the mesh bounds; the simulating world still ignores it
			const bool boundsFallback = !hasCollider && m_Settings.QueryOnly && meshRenderer && meshRenderer->Mesh;

			if (!hasCollider && !boundsFallback)
			{
				continue;
			}

			// The query world mirrors every collider as static so edit-mode picking always works
			RigidBodyType bodyType = RigidBodyType::Static;

			if (rigidBody && !m_Settings.QueryOnly)
			{
				bodyType = rigidBody->Type;
			}

			const float scale = UniformScale(transform.Scale);
			std::vector<ColliderShape> colliderShapes;

			if (boxCollider)
			{
				const glm::vec3 halfExtents = glm::max(glm::abs(boxCollider->HalfExtents * transform.Scale), glm::vec3(0.001f));
				const float convexRadius = std::min(JPH::cDefaultConvexRadius, glm::min(glm::min(halfExtents.x, halfExtents.y), halfExtents.z) * 0.5f);
				const JPH::RefConst<JPH::Shape> box = FinalizeShape(JPH::BoxShapeSettings(ToJolt(halfExtents), convexRadius).Create(), "box");

				if (box)
				{
					colliderShapes.push_back({ box, boxCollider->Offset * transform.Scale });
				}
			}

			if (sphereCollider)
			{
				const float radius = glm::max(sphereCollider->Radius * scale, 0.001f);
				const JPH::RefConst<JPH::Shape> sphere = FinalizeShape(JPH::SphereShapeSettings(radius).Create(), "sphere");

				if (sphere)
				{
					colliderShapes.push_back({ sphere, sphereCollider->Offset * transform.Scale });
				}
			}

			if (capsuleCollider)
			{
				// Jolt capsules are Y-aligned, which is already the engine's up axis - no fix-up rotation
				const float radius = glm::max(capsuleCollider->Radius * scale, 0.001f);
				const float halfHeight = glm::max(capsuleCollider->HalfHeight * scale, 0.001f);
				const JPH::RefConst<JPH::Shape> capsule = FinalizeShape(JPH::CapsuleShapeSettings(halfHeight, radius).Create(), "capsule");

				if (capsule)
				{
					colliderShapes.push_back({ capsule, capsuleCollider->Offset * transform.Scale });
				}
			}

			if (meshCollider && !meshCollider->MeshAsset.empty() && m_Assets)
			{
				const bool convex = meshCollider->Convex || bodyType == RigidBodyType::Dynamic;

				if (!meshCollider->Convex && bodyType == RigidBodyType::Dynamic)
				{
					IG_CORE_WARN("Mesh collider '{}' is triangle-based on a dynamic body, building a convex hull instead", meshCollider->MeshAsset);
				}

				const JPH::RefConst<JPH::Shape> mesh = BuildMeshShape(*m_Implementation, *m_Assets, meshCollider->MeshAsset, convex, transform.Scale);

				if (mesh)
				{
					colliderShapes.push_back({ mesh, meshCollider->Offset * transform.Scale });
				}
			}

			if (boundsFallback)
			{
				const MeshBounds bounds = meshRenderer->Mesh->GetBounds();

				const glm::vec3 halfExtents = glm::max(glm::abs((bounds.Maximum - bounds.Minimum) * 0.5f * transform.Scale), glm::vec3(0.001f));
				const float convexRadius = std::min(JPH::cDefaultConvexRadius, glm::min(glm::min(halfExtents.x, halfExtents.y), halfExtents.z) * 0.5f);
				const JPH::RefConst<JPH::Shape> box = FinalizeShape(JPH::BoxShapeSettings(ToJolt(halfExtents), convexRadius).Create(), "mesh bounds");

				if (box)
				{
					colliderShapes.push_back({ box, (bounds.Maximum + bounds.Minimum) * 0.5f * transform.Scale });
				}
			}

			const JPH::RefConst<JPH::Shape> shape = CombineShapes(colliderShapes);

			if (!shape)
			{
				continue;
			}

			const bool dynamic = bodyType == RigidBodyType::Dynamic;
			const JPH::EMotionType motionType = dynamic ? JPH::EMotionType::Dynamic : (bodyType == RigidBodyType::Kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Static);
			const JPH::ObjectLayer layer = bodyType == RigidBodyType::Static ? Layers::NonMoving : Layers::Moving;

			JPH::BodyCreationSettings bodySettings(shape, ToJolt(transform.Position), ToJoltRotation(transform.Rotation), motionType, layer);

			if (const PhysicsMaterialComponent* materialComponent = registry.try_get<PhysicsMaterialComponent>(handle))
			{
				// Jolt has a single friction value per body; the dynamic coefficient is the one that matters in motion
				bodySettings.mFriction = materialComponent->DynamicFriction;
				bodySettings.mRestitution = materialComponent->Restitution;
			}
			else
			{
				bodySettings.mFriction = 0.6f;
				bodySettings.mRestitution = 0.0f;
			}

			if (dynamic || bodyType == RigidBodyType::Kinematic)
			{
				bodySettings.mLinearDamping = rigidBody ? rigidBody->LinearDamping : 0.01f;
				bodySettings.mAngularDamping = rigidBody ? rigidBody->AngularDamping : 0.05f;
				bodySettings.mGravityFactor = (rigidBody && !rigidBody->UseGravity) ? 0.0f : 1.0f;
			}

			if (dynamic)
			{
				bodySettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
				bodySettings.mMassPropertiesOverride.mMass = rigidBody ? glm::max(rigidBody->Mass, 0.001f) : 1.0f;
			}

			const uint32_t entityID = static_cast<uint32_t>(handle);
			bodySettings.mUserData = entityID;

			const JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(bodySettings, dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);

			if (bodyID.IsInvalid())
			{
				IG_CORE_ERROR("Jolt body creation failed for entity {} (body limit is {})", entityID, MaximumBodies);

				continue;
			}

			PhysicsBody body{};
			body.ID = bodyID;
			body.Dynamic = dynamic;
			body.PreviousPosition = ToJolt(transform.Position);
			body.CurrentPosition = body.PreviousPosition;
			body.PreviousRotation = ToJoltRotation(transform.Rotation);
			body.CurrentRotation = body.PreviousRotation;

			m_Implementation->Bodies.emplace(entityID, body);
		}

		// Query-only worlds never simulate, so the broadphase needs an explicit build before the first raycast
		m_Implementation->System.OptimizeBroadPhase();
	}

	void PhysicsWorld::Step(float fixedTimeStep)
	{
		IG_PROFILE_ZONE();

		if (!IsValid() || m_Settings.QueryOnly || fixedTimeStep <= 0.0f)
		{
			return;
		}

		JPH::BodyInterface& bodyInterface = m_Implementation->System.GetBodyInterface();

		for (auto& [entityID, body] : m_Implementation->Bodies)
		{
			body.PreviousPosition = body.CurrentPosition;
			body.PreviousRotation = body.CurrentRotation;
		}

		m_Implementation->System.Update(fixedTimeStep, 1, m_Implementation->TempAllocator.get(), m_Implementation->JobSystem.get());

		for (auto& [entityID, body] : m_Implementation->Bodies)
		{
			if (!body.Dynamic || body.ID.IsInvalid())
			{
				continue;
			}

			JPH::Vec3 position;
			JPH::Quat rotation;
			bodyInterface.GetPositionAndRotation(body.ID, position, rotation);

			body.CurrentPosition = position;
			body.CurrentRotation = rotation;
		}
	}

	void PhysicsWorld::SyncTransforms(float alpha)
	{
		IG_PROFILE_ZONE();

		if (!IsValid() || !m_Scene || m_Settings.QueryOnly)
		{
			return;
		}

		alpha = glm::clamp(alpha, 0.0f, 1.0f);

		entt::registry& registry = m_Scene->m_Registry->Registry;

		for (const auto& [entityID, body] : m_Implementation->Bodies)
		{
			if (!body.Dynamic)
			{
				continue;
			}

			const entt::entity handle = static_cast<entt::entity>(entityID);

			if (!registry.valid(handle))
			{
				continue;
			}

			const JPH::Vec3 position = body.PreviousPosition + (body.CurrentPosition - body.PreviousPosition) * alpha;
			const JPH::Quat rotation = body.PreviousRotation.SLERP(body.CurrentRotation, alpha);

			TransformComponent& transform = registry.get<TransformComponent>(handle);
			transform.Position = FromJolt(position);
			transform.Rotation = FromJolt(rotation);
		}
	}

	void PhysicsWorld::PushTransform(Entity entity)
	{
		if (!IsValid() || !entity.IsValid())
		{
			return;
		}

		const auto it = m_Implementation->Bodies.find(entity.GetID());

		if (it == m_Implementation->Bodies.end() || it->second.ID.IsInvalid())
		{
			return;
		}

		const TransformComponent& transform = entity.GetTransform();
		const JPH::Vec3 position = ToJolt(transform.Position);
		const JPH::Quat rotation = ToJoltRotation(transform.Rotation);

		JPH::BodyInterface& bodyInterface = m_Implementation->System.GetBodyInterface();
		bodyInterface.SetPositionAndRotation(it->second.ID, position, rotation, JPH::EActivation::Activate);

		if (it->second.Dynamic)
		{
			bodyInterface.SetLinearAndAngularVelocity(it->second.ID, JPH::Vec3::sZero(), JPH::Vec3::sZero());
		}

		it->second.PreviousPosition = position;
		it->second.CurrentPosition = position;
		it->second.PreviousRotation = rotation;
		it->second.CurrentRotation = rotation;
	}

	RaycastHit PhysicsWorld::Raycast(const glm::vec3& origin, const glm::vec3& direction, float maximumDistance) const
	{
		RaycastHit result{};

		if (!IsValid() || glm::length(direction) <= 0.0f)
		{
			return result;
		}

		const JPH::RRayCast ray{ ToJolt(origin), ToJolt(glm::normalize(direction) * maximumDistance) };

		JPH::RayCastResult hit;

		if (!m_Implementation->System.GetNarrowPhaseQuery().CastRay(ray, hit))
		{
			return result;
		}

		result.Hit = true;
		result.Distance = hit.mFraction * maximumDistance;
		result.Position = FromJolt(JPH::Vec3(ray.GetPointOnRay(hit.mFraction)));

		const JPH::BodyLockRead lock(m_Implementation->System.GetBodyLockInterface(), hit.mBodyID);

		if (lock.Succeeded())
		{
			const JPH::Body& body = lock.GetBody();

			result.EntityID = static_cast<uint32_t>(body.GetUserData());
			result.Normal = FromJolt(body.GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, ToJolt(result.Position)));
		}

		return result;
	}

	void PhysicsWorld::SetDebugVisualizationEnabled(bool enabled)
	{
		m_DebugVisualization = enabled;
	}

	bool PhysicsWorld::IsDebugVisualizationEnabled() const
	{
		return m_DebugVisualization;
	}

	void PhysicsWorld::SubmitDebugLines() const
	{
		if (!IsValid() || !m_DebugVisualization)
		{
			return;
		}

#ifdef JPH_DEBUG_RENDERER
		// One renderer for the process: JPH::DebugRenderer keeps a global instance
		static JoltDebugRenderer renderer;

		JPH::BodyManager::DrawSettings drawSettings;
		drawSettings.mDrawShape = true;
		drawSettings.mDrawShapeWireframe = true;
		drawSettings.mDrawBoundingBox = false;

		m_Implementation->System.DrawBodies(drawSettings, &renderer);
#endif
	}

	size_t PhysicsWorld::GetBodyCount() const
	{
		return m_Implementation ? m_Implementation->Bodies.size() : 0;
	}

	bool PhysicsWorld::SelfTest()
	{
		PhysicsContext::Acquire();

		BroadPhaseLayerInterfaceImplementation broadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilterImplementation objectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilterImplementation objectLayerPairFilter;

		JPH::PhysicsSystem system;
		system.Init(1024, 0, 1024, 1024, broadPhaseLayerInterface, objectVsBroadPhaseLayerFilter, objectLayerPairFilter);
		system.SetGravity(JPH::Vec3(0.0f, -9.81f, 0.0f));

		JPH::TempAllocatorImpl tempAllocator(4 * 1024 * 1024);
		JPH::JobSystemThreadPool jobSystem(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, 1);

		JPH::BodyInterface& bodyInterface = system.GetBodyInterface();

		const JPH::RefConst<JPH::Shape> groundShape = JPH::BoxShapeSettings(JPH::Vec3(50.0f, 0.5f, 50.0f)).Create().Get();
		const JPH::BodyID ground = bodyInterface.CreateAndAddBody(JPH::BodyCreationSettings(groundShape, JPH::Vec3(0.0f, -0.5f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NonMoving), JPH::EActivation::DontActivate);

		const JPH::RefConst<JPH::Shape> sphereShape = JPH::SphereShapeSettings(0.5f).Create().Get();
		const JPH::BodyID sphere = bodyInterface.CreateAndAddBody(JPH::BodyCreationSettings(sphereShape, JPH::Vec3(0.0f, 5.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::Moving), JPH::EActivation::Activate);

		system.OptimizeBroadPhase();

		for (int step = 0; step < 240; ++step)
		{
			system.Update(1.0f / 120.0f, 1, &tempAllocator, &jobSystem);
		}

		const float restingHeight = bodyInterface.GetPosition(sphere).GetY();
		const bool settled = std::abs(restingHeight - 0.5f) < 0.05f;

		IG_CORE_INFO("Jolt self-test: sphere came to rest at y = {:.3f} m (expected 0.500)", restingHeight);

		bodyInterface.RemoveBody(sphere);
		bodyInterface.DestroyBody(sphere);
		bodyInterface.RemoveBody(ground);
		bodyInterface.DestroyBody(ground);

		PhysicsContext::Release();

		if (!settled)
		{
			IG_CORE_ERROR("Jolt self-test failed, the physics integration is not behaving");
		}

		return settled;
	}
}