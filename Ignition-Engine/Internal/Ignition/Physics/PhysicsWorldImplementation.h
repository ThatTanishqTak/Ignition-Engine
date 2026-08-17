#pragma once

#include <Jolt/Jolt.h>

#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace Ignition
{
	namespace Layers
	{
		static constexpr JPH::ObjectLayer NonMoving = 0;
		static constexpr JPH::ObjectLayer Moving = 1;
		static constexpr JPH::ObjectLayer Count = 2;
	}

	namespace BroadPhaseLayers
	{
		static constexpr JPH::BroadPhaseLayer NonMoving(0);
		static constexpr JPH::BroadPhaseLayer Moving(1);
		static constexpr JPH::uint Count = 2;
	}

	class ObjectLayerPairFilterImplementation final : public JPH::ObjectLayerPairFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override
		{
			// Static geometry only needs to hear about moving bodies
			return first == Layers::Moving || second == Layers::Moving;
		}
	};

	class BroadPhaseLayerInterfaceImplementation final : public JPH::BroadPhaseLayerInterface
	{
	public:
		BroadPhaseLayerInterfaceImplementation()
		{
			m_ObjectToBroadPhase[Layers::NonMoving] = BroadPhaseLayers::NonMoving;
			m_ObjectToBroadPhase[Layers::Moving] = BroadPhaseLayers::Moving;
		}

		JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::Count; }

		JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override { return m_ObjectToBroadPhase[layer]; }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
		const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
		{
			return layer == BroadPhaseLayers::NonMoving ? "NonMoving" : "Moving";
		}
#endif

	private:
		JPH::BroadPhaseLayer m_ObjectToBroadPhase[Layers::Count]{ BroadPhaseLayers::NonMoving, BroadPhaseLayers::Moving };
	};

	class ObjectVsBroadPhaseLayerFilterImplementation final : public JPH::ObjectVsBroadPhaseLayerFilter
	{
	public:
		bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override
		{
			return layer == Layers::Moving || broadPhaseLayer == BroadPhaseLayers::Moving;
		}
	};

	// Jolt's global registration is process-wide; the editor holds two worlds (query + runtime) at once
	class PhysicsContext
	{
	public:
		static PhysicsContext* Acquire();
		static void Release();

	private:
		uint32_t m_ReferenceCount = 0;
	};

	struct PhysicsBody
	{
		JPH::BodyID ID;
		bool Dynamic = false;

		JPH::Vec3 PreviousPosition = JPH::Vec3::sZero();
		JPH::Quat PreviousRotation = JPH::Quat::sIdentity();
		JPH::Vec3 CurrentPosition = JPH::Vec3::sZero();
		JPH::Quat CurrentRotation = JPH::Quat::sIdentity();
	};

	struct PhysicsWorldImplementation
	{
		PhysicsContext* Context = nullptr;

		// Declared before System so they outlive it - members destruct in reverse order
		BroadPhaseLayerInterfaceImplementation BroadPhaseLayerInterface;
		ObjectVsBroadPhaseLayerFilterImplementation ObjectVsBroadPhaseLayerFilter;
		ObjectLayerPairFilterImplementation ObjectLayerPairFilter;

		JPH::PhysicsSystem System;

		std::unique_ptr<JPH::TempAllocatorImpl> TempAllocator;
		std::unique_ptr<JPH::JobSystemThreadPool> JobSystem;

		std::unordered_map<uint32_t, PhysicsBody> Bodies;

		// Cooked shapes cached by asset path for the lifetime of the world
		std::unordered_map<std::string, JPH::RefConst<JPH::Shape>> ConvexShapes;
		std::unordered_map<std::string, JPH::RefConst<JPH::Shape>> TriangleShapes;

		bool Initialized = false;
	};
}