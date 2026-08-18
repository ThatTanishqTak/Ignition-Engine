#pragma once

#include "Ignition/Core/Export.h"
#include "Ignition/Fluid/FluidTypes.h"

#include <glm/vec2.hpp>

#include <cstdint>
#include <memory>

namespace Ignition
{
	class Renderer;
	struct FluidSolver2DImplementation;

	struct FluidSolver2DSettings
	{
		uint32_t Width = 512;                      // lattice cells along the flow (+X)
		uint32_t Height = 128;                     // lattice cells across the channel (+Y)

		float DomainHeight = 1.0f;                 // m, physical height the lattice spans
		float InletSpeed = 20.0f;                  // m/s
		float AirDensity = 1.225f;                 // kg/m3

		// The 2D lab is driven by Reynolds number rather than air's real viscosity: at 1.48e-5 m2/s a decimetre cylinder sits at Re ~ 1e5, which is nowhere near the vortex-street regime this validates.
		float ReynoldsNumber = 150.0f;
		float LatticeVelocity = 0.06f;             // target inlet speed in lattice units, keep <= 0.1
		float SmagorinskyConstant = 0.16f;

		float ObstacleDiameter = 0.2f;             // m
		glm::vec2 ObstacleCenter{ 0.25f, 0.5f };   // fraction of the domain, (0,0) = inlet floor
	};

	// Forces are per metre of span, as a 2D simulation measures them
	struct FluidForces
	{
		glm::vec2 Force{ 0.0f };
		float Drag = 0.0f;
		float Lift = 0.0f;
		float DragCoefficient = 0.0f;
		float LiftCoefficient = 0.0f;
	};

	// D2Q9 lattice-Boltzmann solver running in Vulkan compute. Steps are queued here and recorded by the renderer at the top of the next frame, before the scene pass opens
	class FluidSolver2D
	{
	public:
		IGNITION_API FluidSolver2D(Renderer* renderer, const FluidSolver2DSettings& settings = {});
		IGNITION_API ~FluidSolver2D();

		FluidSolver2D(const FluidSolver2D&) = delete;
		FluidSolver2D& operator=(const FluidSolver2D&) = delete;

		IGNITION_API bool IsValid() const;

		// Resolution changes rebuild the buffers; everything else applies live
		IGNITION_API void Configure(const FluidSolver2DSettings& settings);
		IGNITION_API const FluidSolver2DSettings& GetSettings() const { return m_Settings; }
		IGNITION_API const FluidLatticeScaling& GetScaling() const { return m_Scaling; }

		IGNITION_API void Reset();
		IGNITION_API void Step(uint32_t steps = 1);

		IGNITION_API void SetVisualizationField(FluidField field);
		IGNITION_API void SetColorScale(float scale);

		IGNITION_API uint64_t GetVisualizationTextureID() const;
		IGNITION_API uint32_t GetVisualizationWidth() const;
		IGNITION_API uint32_t GetVisualizationHeight() const;

		IGNITION_API uint64_t GetStepCount() const;
		IGNITION_API float GetSimulatedTime() const;

		// One frame latent: the reduction lands in a readback buffer the frame after it is dispatched
		IGNITION_API FluidForces GetForces() const;

		IGNITION_API static FluidLatticeScaling ComputeScaling(const FluidSolver2DSettings& settings);

	private:
		Renderer* m_Renderer = nullptr;
		FluidSolver2DSettings m_Settings;
		FluidLatticeScaling m_Scaling;

		std::unique_ptr<FluidSolver2DImplementation> m_Implementation;
	};
}