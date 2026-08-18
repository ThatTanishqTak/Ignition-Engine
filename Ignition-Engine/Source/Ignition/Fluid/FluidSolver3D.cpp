#include "Ignition/Fluid/FluidSolver3D.h"

#include "Ignition/Fluid/FluidSolver3DImplementation.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/RendererImplementation.h"
#include "Ignition/Core/Log.h"

#include <algorithm>
#include <cmath>

namespace Ignition
{
	FluidSolver3D::FluidSolver3D(Renderer* renderer, const FluidSolver3DSettings& settings) : m_Renderer(renderer), m_Implementation(std::make_unique<FluidSolver3DImplementation>())
	{
		Configure(settings);
	}

	FluidSolver3D::~FluidSolver3D() = default;

	FluidLatticeScaling FluidSolver3D::ComputeScaling(const FluidSolver3DSettings& settings)
	{
		FluidLatticeScaling scaling{};

		const glm::uvec3 resolution = glm::max(settings.Resolution, glm::uvec3(16));
		const glm::vec3 domain = glm::max(settings.DomainSize, glm::vec3(1e-3f));
		const float inletSpeed = std::max(settings.InletSpeed, 1e-4f);
		const float referenceLength = std::max(settings.ReferenceLength, 1e-4f);

		// Cells are cubic, so one axis has to win. The largest per-axis requirement does, which keeps the requested domain inside the lattice rather than cropping it
		scaling.CellSize = std::max({ domain.x / static_cast<float>(resolution.x), domain.y / static_cast<float>(resolution.y), domain.z / static_cast<float>(resolution.z) });
		scaling.LatticeVelocity = std::clamp(settings.LatticeVelocity, 0.001f, 0.2f);
		scaling.TimeStep = scaling.LatticeVelocity * scaling.CellSize / inletSpeed;

		scaling.ObstacleDiameterCells = referenceLength / scaling.CellSize;

		// The physical viscosity returns here, as the Conventions table promised: nu_lattice = nu * dt / dx^2
		scaling.LatticeViscosity = settings.KinematicViscosity * scaling.TimeStep / std::max(scaling.CellSize * scaling.CellSize, 1e-12f);

		const float requestedTau = 3.0f * scaling.LatticeViscosity + 0.5f;
		const float relaxationFloor = std::max(settings.MinimumRelaxationTime, 0.5001f);

		// At Re ~ 1e7 the molecular viscosity is orders of magnitude below what a few hundred cells can resolve, so tau lands on 0.5 and the scheme goes inviscid.
		// Clamping it and letting Smagorinsky supply the rest is what a wall-modelled LES does; hiding the clamp is what makes a wrong answer look right
		scaling.SubgridLimited = requestedTau < relaxationFloor;
		scaling.RelaxationTime = std::max(requestedTau, relaxationFloor);

		scaling.KinematicViscosity = settings.KinematicViscosity;
		scaling.EffectiveViscosity = (scaling.RelaxationTime - 0.5f) / 3.0f * scaling.CellSize * scaling.CellSize / std::max(scaling.TimeStep, 1e-12f);

		scaling.ReynoldsNumber = inletSpeed * referenceLength / std::max(settings.KinematicViscosity, 1e-12f);

		// Lattice force -> Newtons: density * length^4 / time^2. The 2D lab's scale carries one power of length less because it reports per metre of span
		const float cellSize = scaling.CellSize;
		scaling.ForceScale = settings.AirDensity * cellSize * cellSize * cellSize * cellSize / std::max(scaling.TimeStep * scaling.TimeStep, 1e-24f);

		// The relaxation floor makes the first condition true by construction; the other two are the ones that still fail
		scaling.Stable = scaling.RelaxationTime > 0.5005f && scaling.LatticeVelocity <= 0.1f && scaling.ObstacleDiameterCells >= 8.0f;

		return scaling;
	}

	void FluidSolver3D::Configure(const FluidSolver3DSettings& settings)
	{
		VulkanRenderer* backend = m_Renderer ? m_Renderer->m_Implementation->Backend : nullptr;

		const bool rebuild = !m_Implementation->Handle || settings.Resolution != m_Settings.Resolution;

		m_Settings = settings;
		m_Scaling = ComputeScaling(settings);

		if (!backend)
		{
			return;
		}

		if (!rebuild)
		{
			m_Implementation->Handle->Configure(settings);

			return;
		}

		if (m_Implementation->Handle)
		{
			backend->Retire(std::move(m_Implementation->Handle));
		}

		m_Implementation->Handle = backend->CreateFluidSolver3D(settings);
		m_Implementation->Backend = backend->GetSelfReference();

		if (!m_Implementation->Handle)
		{
			IG_CORE_ERROR("Wind tunnel creation failed ({}x{}x{})", settings.Resolution.x, settings.Resolution.y, settings.Resolution.z);
		}
	}

	bool FluidSolver3D::IsValid() const
	{
		return m_Implementation->Handle && m_Implementation->Handle->IsValid();
	}

	void FluidSolver3D::Reset()
	{
		if (m_Implementation->Handle)
		{
			m_Implementation->Handle->RequestReset();
		}
	}

	void FluidSolver3D::Step(uint32_t steps)
	{
		if (m_Implementation->Handle)
		{
			m_Implementation->Handle->RequestSteps(steps);
		}
	}

	uint64_t FluidSolver3D::GetStepCount() const
	{
		return m_Implementation->Handle ? m_Implementation->Handle->GetStepCount() : 0;
	}

	float FluidSolver3D::GetSimulatedTime() const
	{
		return static_cast<float>(GetStepCount()) * m_Scaling.TimeStep;
	}

	AeroForces FluidSolver3D::GetForces() const
	{
		AeroForces forces{};

		if (!m_Implementation->Handle)
		{
			return forces;
		}

		forces.Force = m_Implementation->Handle->GetLatticeForce() * m_Scaling.ForceScale;

		// Torque is a force times a lattice length, so it takes one more factor of dx than the force does
		forces.Torque = m_Implementation->Handle->GetLatticeTorque() * m_Scaling.ForceScale * m_Scaling.CellSize;

		// Frontal area counted from the mask rather than assumed, so it stays right once the voxelizer replaces the sphere
		forces.ReferenceArea = static_cast<float>(m_Implementation->Handle->GetProjectedCellCount()) * m_Scaling.CellSize * m_Scaling.CellSize;

		// Air flows in -Z: drag is the push downstream, downforce is the push into the road
		forces.Drag = -forces.Force.z;
		forces.Downforce = -forces.Force.y;
		forces.SideForce = forces.Force.x;

		const float dynamicPressure = 0.5f * m_Settings.AirDensity * m_Settings.InletSpeed * m_Settings.InletSpeed * forces.ReferenceArea;

		if (dynamicPressure > 0.0f)
		{
			forces.DragCoefficient = forces.Drag / dynamicPressure;
			forces.LiftCoefficient = forces.Force.y / dynamicPressure;
		}

		// A pitching moment about X splits the downforce between the axles: front at +Z, rear at -Z, both a half wheelbase from the reference point
		const float wheelbase = std::max(m_Settings.Wheelbase, 1e-3f);

		if (std::abs(forces.Downforce) > 1e-3f)
		{
			forces.FrontBalance = 0.5f * (1.0f + 2.0f * forces.Torque.x / (wheelbase * forces.Downforce));
		}

		return forces;
	}
}