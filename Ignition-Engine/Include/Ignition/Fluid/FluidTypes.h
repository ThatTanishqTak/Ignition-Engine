#pragma once

#include <glm/vec3.hpp>

#include <cstdint>

namespace Ignition
{
	enum class FluidField
	{
		VelocityMagnitude = 0,
		Vorticity,
		Pressure
	};

	enum class FluidSliceAxis
	{
		X = 0, // side view - the classic tunnel profile
		Y,     // top view
		Z      // front view, looking into the wind
	};

	struct FluidLatticeScaling
	{
		float CellSize = 0.0f;             // m per cell
		float TimeStep = 0.0f;             // s per lattice step
		float LatticeVelocity = 0.0f;
		float LatticeViscosity = 0.0f;
		float RelaxationTime = 0.0f;
		float ReferenceLengthCells = 0.0f; // the reference length, in cells
		float KinematicViscosity = 0.0f;   // m2/s asked for
		float EffectiveViscosity = 0.0f;   // m2/s the lattice actually carries after the relaxation-time floor
		float ReynoldsNumber = 0.0f;
		float ForceScale = 0.0f;           // lattice force -> Newtons
		bool Stable = false;
		bool SubgridLimited = false;       // molecular viscosity fell below what the lattice resolves; Smagorinsky is carrying it
	};

	// The tunnel is the scene, not a box inside it: the lattice floor is the world's y = 0 plane centred on the origin, and air flows in -Z so the car faces the wind
	struct FluidSolver3DSettings
	{
		glm::uvec3 Resolution{ 128, 64, 256 };        // X across, Y up, Z along the flow
		glm::vec3 DomainSize{ 4.0f, 2.0f, 8.0f };     // m; cells are cubic, so the lattice covers at least this
		glm::vec3 ReferencePoint{ 0.0f };             // world position torque is taken about

		float InletSpeed = 40.0f;                     // m/s
		float AirDensity = 1.225f;                    // kg/m3
		float KinematicViscosity = 1.48e-5f;          // m2/s
		float SmagorinskyConstant = 0.16f;
		float LatticeVelocity = 0.06f;                // target inlet speed in lattice units, keep <= 0.1

		// Real air puts a car at Re ~ 1e7, which drives tau onto 0.5 and the solver onto the edge of stability. This floor is what the subgrid model relaxes around
		float MinimumRelaxationTime = 0.503f;

		float ReferenceLength = 1.0f;                 // m, sets the reported Reynolds number and the cells-across check
		float Wheelbase = 3.6f;                       // m, the moment arm aero balance is split over

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
		bool VoxelDebugView = false;                  // the shell colours by object id instead of pressure - check the silhouette before trusting a force

		// Line sweeps travel a full axis per pass, so a handful of iterations turns every corner a car presents
		uint32_t FloodIterations = 8;

		bool operator==(const FluidSolver3DSettings&) const = default;
	};

	// What the voxelizer actually produced. Check the silhouette against this before believing any force number
	struct FluidVoxelStatus
	{
		uint32_t BodyCount = 0;
		uint32_t TriangleCount = 0;
		uint32_t SolidCells = 0;
		uint32_t FloodResidual = 0;     // cells the final flood iteration was still changing; nonzero means it did not converge
		uint32_t RejectedTriangles = 0; // spanned more cells than one thread should walk
		bool Voxelized = false;         // false when the tunnel holds no geometry at all
	};

	// Newtons, not Newtons per metre. Getting that one power of length wrong produces coefficients that look plausible
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
}