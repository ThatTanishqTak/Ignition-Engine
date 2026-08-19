#pragma once

#include "Ignition/Renderer/Material.h"

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include <memory>
#include <string>
#include <cstdint>

namespace Ignition
{
	class Mesh;

	struct TagComponent
	{
		std::string Name;
	};

	struct TransformComponent
	{
		glm::vec3 Position{ 0.0f };
		glm::quat Rotation{ 1.0f, 0.0f, 0.0f, 0.0f }; // identity, stored w-first
		glm::vec3 Scale{ 1.0f };

		glm::mat4 GetMatrix() const
		{
			return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale);
		}

		// Euler angles are a presentation detail: convert at the UI boundary, never store them
		glm::vec3 GetEulerAngles() const
		{
			return glm::eulerAngles(Rotation);
		}

		void SetEulerAngles(const glm::vec3& radians)
		{
			Rotation = glm::normalize(glm::quat(radians));
		}
	};

	struct MeshRendererComponent
	{
		std::shared_ptr<Ignition::Mesh> Mesh;
		Ignition::Material Material;

		std::string MeshAsset;
		std::string AlbedoAsset;
	};

	// The tunnel is a scene object like anything else: serialized, inspectable, gizmo-positionable.
	// The entity's position is the centre of the floor, so the rolling road sits exactly on it
	struct WindTunnelComponent
	{
		glm::uvec3 Resolution{ 128, 64, 256 };        // X across, Y up, Z along the flow
		glm::vec3 DomainSize{ 4.0f, 2.0f, 8.0f };     // m; cells are cubic, so the lattice covers at least this

		float InletSpeed = 40.0f;                     // m/s, air flows in -Z
		float AirDensity = 1.225f;                    // kg/m3
		float KinematicViscosity = 1.48e-5f;          // m2/s
		float SmagorinskyConstant = 0.16f;
		float LatticeVelocity = 0.06f;
		float MinimumRelaxationTime = 0.503f;         // the floor Smagorinsky relaxes around once air's viscosity outruns the lattice

		float ReferenceLength = 1.0f;                 // m, sets the reported Reynolds number
		float Wheelbase = 3.6f;                       // m, the moment arm aero balance is split over
		glm::vec3 ReferencePoint{ 0.0f };             // relative to the entity, torque is taken about it

		// Analytic sphere until the voxelizer lands - the second rung of the ladder needs no mesh, and it keeps solver bugs separable from voxelizer bugs
		float ObstacleDiameter = 1.0f;                // m
		glm::vec3 ObstacleCenter{ 0.5f, 0.5f, 0.7f }; // fraction of the lattice

		bool RollingRoad = true;
		bool DrawBounds = true;

		// Visualization (Step 3.4) - all live, none reset the flow
		bool SliceEnabled = true;
		int SliceAxis = 0;                            // 0 = X (side), 1 = Y (top), 2 = Z (front)
		float SlicePosition = 0.5f;                   // fraction along the axis
		int SliceField = 0;                           // 0 = velocity, 1 = vorticity, 2 = pressure
		float ColorScale = 1.0f;
		float SliceOpacity = 0.85f;
		bool ParticlesEnabled = true;
		uint32_t ParticleCount = 100000;
		bool SurfacePressureEnabled = false;
		bool VoxelDebugView = false;

		// Voxelizer (Step 3.2)
		uint32_t FloodIterations = 8;
	};

	// The mesh the wind actually sees. Rarely the mesh you draw: the CFD mesh is decimated and sealed, because
	// open bodywork leaks the interior flood fill and sub-centimetre greebles alias into noise at any usable voxel size
	struct AeroBodyComponent
	{
		std::shared_ptr<Ignition::Mesh> Mesh;   // resolved from MeshAsset; the visual mesh stands in when it is empty
		std::string MeshAsset;

		uint32_t ObjectID = 1;                  // 1-255, stamped into the voxel mask so Phase 4 can spin wheels without re-voxelizing
		bool Enabled = true;
	};
}