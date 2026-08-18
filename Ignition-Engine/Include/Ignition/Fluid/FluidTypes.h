#pragma once

namespace Ignition
{
	enum class FluidField
	{
		VelocityMagnitude = 0,
		Vorticity,
		Density
	};

	enum class FluidSliceAxis
	{
		X = 0, // side view - the classic tunnel profile
		Y,     // top view
		Z      // front view, looking into the wind
	};

	struct FluidLatticeScaling
	{
		float CellSize = 0.0f; // m per cell
		float TimeStep = 0.0f; // s per lattice step
		float LatticeVelocity = 0.0f;
		float LatticeViscosity = 0.0f;
		float RelaxationTime = 0.0f;
		float ObstacleDiameterCells = 0.0f; // reference length, in cells
		float KinematicViscosity = 0.0f; // m2/s asked for
		float EffectiveViscosity = 0.0f; // m2/s the lattice actually carries after the relaxation-time floor
		float ReynoldsNumber = 0.0f;
		float ForceScale = 0.0f; // lattice force -> N per metre of span (2D) or N (3D)
		bool Stable = false;
		bool SubgridLimited = false; // molecular viscosity fell below what the lattice resolves; Smagorinsky is carrying it
	};
}