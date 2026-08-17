#pragma once

#include <vulkan/vulkan.h>

#if defined(IGNITION_PROFILE)
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>
#endif

namespace Ignition
{
#if defined(IGNITION_PROFILE)
	using GPUProfilerContext = TracyVkCtx;
#else
	using GPUProfilerContext = void*;
#endif
}

#if defined(IGNITION_PROFILE)
#define IG_PROFILE_GPU_CREATE(context, physicalDevice, device, queue, commandBuffer) context = TracyVkContext(physicalDevice, device, queue, commandBuffer)
#define IG_PROFILE_GPU_NAME(context, name) TracyVkContextName(context, name, static_cast<uint16_t>(sizeof(name) - 1))
#define IG_PROFILE_GPU_DESTROY(context) do { if (context) { TracyVkDestroy(context); context = nullptr; } } while (false)
#define IG_PROFILE_GPU_ZONE(context, commandBuffer, name) TracyVkZone(context, commandBuffer, name)
#define IG_PROFILE_GPU_COLLECT(context, commandBuffer) TracyVkCollect(context, commandBuffer)
#else
#define IG_PROFILE_GPU_CREATE(context, physicalDevice, device, queue, commandBuffer) ((void)0)
#define IG_PROFILE_GPU_NAME(context, name) ((void)0)
#define IG_PROFILE_GPU_DESTROY(context) ((void)0)
#define IG_PROFILE_GPU_ZONE(context, commandBuffer, name) ((void)0)
#define IG_PROFILE_GPU_COLLECT(context, commandBuffer) ((void)0)
#endif