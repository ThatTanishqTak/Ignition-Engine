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