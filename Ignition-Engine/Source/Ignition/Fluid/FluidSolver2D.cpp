#include "Ignition/Fluid/FluidSolver2D.h"

#include "Ignition/Fluid/FluidSolver2DImplementation.h"
#include "Ignition/Renderer/Renderer.h"
#include "Ignition/Renderer/RendererImplementation.h"
#include "Ignition/Core/Log.h"

#include <algorithm>
#include <cmath>

namespace Ignition
{
	FluidSolver2D::FluidSolver2D(Renderer* renderer, const FluidSolver2DSettings& settings) : m_Renderer(renderer), m_Implementation(std::make_unique<FluidSolver2DImplementation>())
	{
		Configure(settings);
	}

	FluidSolver2D::~FluidSolver2D() = default;

	FluidLatticeScaling FluidSolver2D::ComputeScaling(const FluidSolver2DSettings& settings)
	{
		FluidLatticeScaling scaling{};

		const float domainHeight = std::max(settings.DomainHeight, 1e-4f);
		const float inletSpeed = std::max(settings.InletSpeed, 1e-4f);
		const uint32_t cellsY = std::max(settings.Height, 4u);

		scaling.CellSize = domainHeight / static_cast<float>(cellsY);
		scaling.LatticeVelocity = std::clamp(settings.LatticeVelocity, 0.001f, 0.2f);
		scaling.TimeStep = scaling.LatticeVelocity * scaling.CellSize / inletSpeed;

		scaling.ObstacleDiameterCells = std::max(settings.ObstacleDiameter, 1e-4f) / scaling.CellSize;
		scaling.ReynoldsNumber = std::max(settings.ReynoldsNumber, 1.0f);

		// Re = u_lattice * D_lattice / nu_lattice, solved for the viscosity the lattice must carry
		scaling.LatticeViscosity = scaling.LatticeVelocity * scaling.ObstacleDiameterCells / scaling.ReynoldsNumber;
		scaling.RelaxationTime = 3.0f * scaling.LatticeViscosity + 0.5f;
		scaling.KinematicViscosity = scaling.LatticeViscosity * scaling.CellSize * scaling.CellSize / std::max(scaling.TimeStep, 1e-12f);

		// The lab derives viscosity from the requested Reynolds number, so the lattice always carries exactly what was asked for - nothing is ever clamped away
		scaling.EffectiveViscosity = scaling.KinematicViscosity;
		scaling.SubgridLimited = false;

		// Lattice force -> N per metre of span: density * length^3 / time^2 in two dimensions
		scaling.ForceScale = settings.AirDensity * scaling.CellSize * scaling.CellSize * scaling.CellSize / std::max(scaling.TimeStep * scaling.TimeStep, 1e-24f);

		scaling.Stable = scaling.RelaxationTime > 0.51f && scaling.LatticeVelocity <= 0.1f && scaling.ObstacleDiameterCells >= 8.0f;

		return scaling;
	}

	void FluidSolver2D::Configure(const FluidSolver2DSettings& settings)
	{
		VulkanRenderer* backend = m_Renderer ? m_Renderer->m_Implementation->Backend : nullptr;

		const bool rebuild = !m_Implementation->Handle || settings.Width != m_Settings.Width || settings.Height != m_Settings.Height;

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

		m_Implementation->Handle = backend->CreateFluidSolver2D(settings);
		m_Implementation->Backend = backend->GetSelfReference();

		if (!m_Implementation->Handle)
		{
			IG_CORE_ERROR("Fluid solver creation failed ({}x{})", settings.Width, settings.Height);
		}
	}

	bool FluidSolver2D::IsValid() const
	{
		return m_Implementation->Handle && m_Implementation->Handle->IsValid();
	}

	void FluidSolver2D::Reset()
	{
		if (m_Implementation->Handle)
		{
			m_Implementation->Handle->RequestReset();
		}
	}

	void FluidSolver2D::Step(uint32_t steps)
	{
		if (m_Implementation->Handle)
		{
			m_Implementation->Handle->RequestSteps(steps);
		}
	}

	void FluidSolver2D::SetVisualizationField(FluidField field)
	{
		if (m_Implementation->Handle)
		{
			m_Implementation->Handle->SetVisualizationField(field);
		}
	}

	void FluidSolver2D::SetColorScale(float scale)
	{
		if (m_Implementation->Handle)
		{
			m_Implementation->Handle->SetColorScale(scale);
		}
	}

	uint64_t FluidSolver2D::GetVisualizationTextureID() const
	{
		return m_Implementation->Handle ? m_Implementation->Handle->GetVisualizationTextureID() : 0;
	}

	uint32_t FluidSolver2D::GetVisualizationWidth() const
	{
		return m_Settings.Width;
	}

	uint32_t FluidSolver2D::GetVisualizationHeight() const
	{
		return m_Settings.Height;
	}

	uint64_t FluidSolver2D::GetStepCount() const
	{
		return m_Implementation->Handle ? m_Implementation->Handle->GetStepCount() : 0;
	}

	float FluidSolver2D::GetSimulatedTime() const
	{
		return static_cast<float>(GetStepCount()) * m_Scaling.TimeStep;
	}

	FluidForces FluidSolver2D::GetForces() const
	{
		FluidForces forces{};

		if (!m_Implementation->Handle)
		{
			return forces;
		}

		forces.Force = m_Implementation->Handle->GetLatticeForce() * m_Scaling.ForceScale;
		forces.Drag = forces.Force.x;
		forces.Lift = forces.Force.y;

		// Cd = 2F / (rho v^2 D), with F per metre of span and D the cylinder diameter
		const float dynamicPressure = 0.5f * m_Settings.AirDensity * m_Settings.InletSpeed * m_Settings.InletSpeed * std::max(m_Settings.ObstacleDiameter, 1e-4f);

		if (dynamicPressure > 0.0f)
		{
			forces.DragCoefficient = forces.Drag / dynamicPressure;
			forces.LiftCoefficient = forces.Lift / dynamicPressure;
		}

		return forces;
	}
}