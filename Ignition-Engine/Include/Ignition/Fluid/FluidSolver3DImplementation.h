#pragma once

#include "Ignition/Fluid/FluidSolver3D.h"
#include "Ignition/Renderer/ResourceImplementation.h"
#include "Ignition/Renderer/Vulkan/VulkanFluidSolver3D.h"

namespace Ignition
{
	struct FluidSolver3DImplementation : ResourceImplementation<VulkanFluidSolver3D>
	{

	};
}