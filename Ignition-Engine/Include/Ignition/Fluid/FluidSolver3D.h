#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Fluid/FluidTypes.h"
#include "Ignition/Renderer/Mesh.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace Ignition
{
	class Renderer;
	struct FluidSolver3DImplementation;

	// One body the wind sees. Geometry is Mesh::GetGeometry() - the CPU-side copy Phase 1 added for collider cooking, second consumer exactly as planned
	struct FluidBody
	{
		const MeshGeometry* Geometry = nullptr;
		glm::mat4 Transform{ 1.0f };  // model -> world
		uint32_t ObjectID = 1;        // 1-255, stamped into the mask so Phase 4 can spin wheels without re-voxelizing

		bool operator==(const FluidBody&) const = default;
	};

	// What the voxelizer actually produced. Check the silhouette against this before believing any force number
	struct FluidVoxelStatus
	{
		uint32_t BodyCount = 0;
		uint32_t TriangleCount = 0;
		uint32_t SolidCells = 0;
		uint32_t FloodResidual = 0;        // cells the final flood iteration was still changing; nonzero means it did not converge
		uint32_t RejectedTriangles = 0;    // spanned more cells than one thread should walk
		bool Voxelized = false;            // false when no body is bound and the analytic sphere is standing in
	};

	// The tunnel's authored state in absolute units. Air flows in -Z, so the inlet is the +Z face and the car faces the wind
	struct FluidSolver3DSettings
	{
		glm::uvec3 Resolution{ 128, 64, 256 };        // X across, Y up, Z along the flow
		glm::vec3 DomainSize{ 4.0f, 2.0f, 8.0f };     // m; cells are cubic, so the lattice covers at least this

		glm::vec3 Origin{ 0.0f };                     // world position of the floor centre - the rolling road sits exactly here
		glm::vec3 ReferencePoint{ 0.0f };             // world position torque is taken about

		float InletSpeed = 40.0f;                     // m/s
		float AirDensity = 1.225f;                    // kg/m3
		float KinematicViscosity = 1.48e-5f;          // m2/s - real air, unlike the 2D lab
		float SmagorinskyConstant = 0.16f;
		float LatticeVelocity = 0.06f;                // target inlet speed in lattice units, keep <= 0.1

		// Real air puts a car at Re ~ 1e7, which drives tau onto 0.5 and the solver onto the edge of stability.
		// This floor is what the subgrid model relaxes around; it is the 3D answer to the 2D lab's Reynolds-driven viscosity
		float MinimumRelaxationTime = 0.503f;

		float ReferenceLength = 1.0f;                 // m, sets the reported Reynolds number and the cells-across check
		float Wheelbase = 3.6f;                       // m, the moment arm aero balance is split over

		// Analytic sphere until the voxelizer (Step 3.2) writes the same mask from a mesh
		float ObstacleDiameter = 1.0f;                // m
		glm::vec3 ObstacleCenter{ 0.5f, 0.5f, 0.7f }; // fraction of the lattice, (0,0,0) = floor at the outlet corner

		bool RollingRoad = true;

		bool SliceEnabled = true;
		FluidSliceAxis SliceAxis = FluidSliceAxis::X;
		float SlicePosition = 0.5f;                   // fraction along the axis
		FluidField SliceField = FluidField::VelocityMagnitude;
		float ColorScale = 1.0f;
		float SliceOpacity = 0.85f;

		bool ParticlesEnabled = true;
		uint32_t ParticleCount = 100000;              // clamped to the solver's capacity

		bool SurfacePressureEnabled = false;
		bool VoxelDebugView = false;   // the shell colours by object id instead of pressure - check the silhouette before trusting a force

		// Voxelizer (Step 3.2). Line sweeps travel a full axis per pass, so a handful of iterations turns every corner a car presents
		uint32_t FloodIterations = 8;

		bool operator==(const FluidSolver3DSettings&) const = default;
	};

	// Newtons, not Newtons per metre. That one power of length is the difference between the 2D lab and this, and getting it wrong produces coefficients that look plausible
	struct AeroForces
	{
		glm::vec3 Force{ 0.0f };      // N, world axes
		glm::vec3 Torque{ 0.0f };     // N m about the reference point

		float Drag = 0.0f;            // N downstream, along -Z
		float Downforce = 0.0f;       // N, positive presses the body into the road
		float SideForce = 0.0f;       // N along +X

		float DragCoefficient = 0.0f;
		float LiftCoefficient = 0.0f;

		float FrontBalance = 0.0f;    // fraction of the downforce carried by the front axle
		float ReferenceArea = 0.0f;   // m2, counted from inlet-facing solid columns rather than assumed
	};

	// D3Q19 lattice-Boltzmann tunnel running in Vulkan compute. Steps are queued here and recorded by the renderer at the top of the next frame, before the scene pass opens
	class FluidSolver3D
	{
	public:
		IGNITION_API FluidSolver3D(Renderer* renderer, const FluidSolver3DSettings& settings = {});
		IGNITION_API ~FluidSolver3D();

		FluidSolver3D(const FluidSolver3D&) = delete;
		FluidSolver3D& operator=(const FluidSolver3D&) = delete;

		IGNITION_API bool IsValid() const;

		// Resolution changes rebuild the lattice; everything else applies live, and obstacle changes reset
		IGNITION_API void Configure(const FluidSolver3DSettings& settings);
		IGNITION_API const FluidSolver3DSettings& GetSettings() const { return m_Settings; }
		IGNITION_API const FluidLatticeScaling& GetScaling() const { return m_Scaling; }

		IGNITION_API void Reset();
		IGNITION_API void Step(uint32_t steps = 1);

		// Idempotent: an unchanged list costs a comparison. Any real change re-voxelizes, which resets the flow
		IGNITION_API void SetBodies(const std::vector<FluidBody>& bodies);
		IGNITION_API FluidVoxelStatus GetVoxelStatus() const;

		IGNITION_API uint64_t GetStepCount() const;
		IGNITION_API float GetSimulatedTime() const;

		// Panel preview of the slice plane; zero until the first frame has rendered it
		IGNITION_API uint64_t GetSliceTextureID() const;

		// One frame latent: the reduction lands in a readback buffer the frame after it is dispatched
		IGNITION_API AeroForces GetForces() const;

		IGNITION_API static FluidLatticeScaling ComputeScaling(const FluidSolver3DSettings& settings);

	private:
		Renderer* m_Renderer = nullptr;
		FluidSolver3DSettings m_Settings;
		FluidLatticeScaling m_Scaling;

		std::unique_ptr<FluidSolver3DImplementation> m_Implementation;
	};
}