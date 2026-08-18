#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
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

	// Mirrors FluidParameters in Fluid2D.slang
	struct FluidPushConstants
	{
		glm::uvec2 Resolution{ 0, 0 };
		glm::vec2 ObstacleCenter{ 0.0f };
		float ObstacleRadius = 0.0f;
		float LatticeVelocity = 0.0f;
		float RelaxationTime = 0.0f;
		float SmagorinskyConstant = 0.0f;
		uint32_t ReductionGroups = 0;
		uint32_t Field = 0;
		float ColorScale = 1.0f;
		uint32_t Padding = 0;
	};

	static_assert(sizeof(FluidPushConstants) == 48, "Fluid push constant block must stay 48 bytes to match Fluid2D.slang");

	// Mirrors FluidParameters3D in Fluid3D.slang. Every vec3 sits on a 16-byte boundary so the C++ offsets and the shader's std140 rows agree
	struct FluidPushConstants3D
	{
		glm::uvec3 Resolution{ 0, 0, 0 };
		uint32_t ReductionGroups = 0;
		glm::vec3 ObstacleCenter{ 0.0f };          // lattice cells
		float ObstacleRadius = 0.0f;               // lattice cells
		glm::vec3 ReferencePoint{ 0.0f };          // lattice cells, torque is taken about this
		float LatticeVelocity = 0.0f;
		float RelaxationTime = 0.0f;
		float SmagorinskyConstant = 0.0f;
		uint32_t Flags = 0;                        // bit 0: rolling road
		uint32_t Padding = 0;

		// Visualization (Step 3.4)
		uint32_t SliceAxis = 0;
		uint32_t SliceField = 0;
		float SliceFraction = 0.5f;
		float ColorScale = 1.0f;
		uint32_t ParticleCount = 0;
		uint32_t FrameSeed = 0;
		uint32_t ShellVertexCapacity = 0;
		float AdvectSteps = 0.0f;                  // lattice steps this frame - zero while paused, which freezes the tracers
	};

	static_assert(sizeof(FluidPushConstants3D) == 96, "Wind tunnel push constant block must stay 96 bytes to match Fluid3D.slang");
}