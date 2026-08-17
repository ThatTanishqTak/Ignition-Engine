#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
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
}