#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

namespace Ignition
{
	struct PushConstantData
	{
		glm::mat4 MVP;
		glm::vec4 Tint;
	};

	static_assert(sizeof(PushConstantData) == 80, "Push constant block must stay 80 bytes to match the layout expected by the shaders");

	// Mirrors FluidParameters3D in Fluid3D.slang. Every vec3 sits on a 16-byte boundary so the C++ offsets and the shader's std140 rows agree
	struct FluidPushConstants3D
	{
		glm::uvec3 Resolution{ 0, 0, 0 };
		uint32_t ReductionGroups = 0;
		glm::vec3 ReferencePoint{ 0.0f };          // lattice cells, torque is taken about this
		float LatticeVelocity = 0.0f;
		float RelaxationTime = 0.0f;
		float SmagorinskyConstant = 0.0f;
		uint32_t Flags = 0;                        // bit 0: rolling road, bit 1: the shell colours by object id instead of pressure
		uint32_t FloodAxis = 0;                    // 0 = X, 1 = Y, 2 = Z

		// Visualization (Step 3.4)
		uint32_t SliceAxis = 0;
		uint32_t SliceField = 0;
		float SliceFraction = 0.5f;
		float ColorScale = 1.0f;
		uint32_t ParticleCount = 0;
		uint32_t FrameSeed = 0;
		uint32_t ShellVertexCapacity = 0;
		float AdvectSteps = 0.0f;                  // lattice steps this frame - zero while paused, which freezes the tracers

		// Voxelizer (Step 3.2)
		uint32_t TriangleOffset = 0;
		uint32_t TriangleCount = 0;
		uint32_t VoxelObjectID = 1;
		uint32_t VoxelBudget = 0;
	};

	static_assert(sizeof(FluidPushConstants3D) == 96, "Wind tunnel push constant block must stay 96 bytes to match Fluid3D.slang");

	// Mirrors VolumeParameters in FluidVolume.slang. 128 bytes is the guaranteed push-constant minimum, and this fills it exactly
	struct FluidVolumePushConstants
	{
		glm::mat4 InverseViewProjection{ 1.0f };

		glm::vec3 LatticeMinimum{ 0.0f };
		float CellSize = 0.0f;

		glm::uvec3 Resolution{ 0, 0, 0 };
		uint32_t Field = 0;

		float LatticeVelocity = 0.0f;
		float ColorScale = 1.0f;
		float Density = 0.0f;                      // extinction per metre at full departure from freestream
		float Threshold = 0.0f;                    // departure below this is perfectly clear air

		uint32_t StepCount = 0;
		uint32_t Flags = 0;                        // bit 0: solid cells shade as the voxel mask
		uint32_t Padding0 = 0;
		uint32_t Padding1 = 0;
	};

	static_assert(sizeof(FluidVolumePushConstants) == 128, "Volume push constant block must stay 128 bytes to match FluidVolume.slang");
}