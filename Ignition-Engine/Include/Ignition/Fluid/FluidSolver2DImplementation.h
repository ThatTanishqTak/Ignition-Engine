#pragma once

#include "Ignition/Fluid/FluidSolver2D.h"
#include "Ignition/Renderer/ResourceImplementation.h"
#include "Ignition/Renderer/Vulkan/VulkanFluidSolver2D.h"

namespace Ignition
{
	struct FluidSolver2DImplementation : ResourceImplementation<VulkanFluidSolver2D>
	{

	};
}