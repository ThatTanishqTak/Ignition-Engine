// Shared by Fluid3D.slang's slice and shell kernels and by FluidVolume.slang's ray march, so a plane, a shell and the
// volume can never disagree about what a number looks like. The .inc.slang suffix keeps this out of the shader glob
#ifndef IGNITION_FLUID_COLOR
#define IGNITION_FLUID_COLOR

// Linear values throughout; the composite pass does the sRGB encode, matching VulkanImGui's convention
float3 Ramp(float value)
{
    const float scaled = saturate(value) * 4.0;

    if (scaled < 1.0)
    {
        return lerp(float3(0.05, 0.02, 0.20), float3(0.10, 0.35, 0.75), scaled);
    }

    if (scaled < 2.0)
    {
        return lerp(float3(0.10, 0.35, 0.75), float3(0.10, 0.75, 0.60), scaled - 1.0);
    }

    if (scaled < 3.0)
    {
        return lerp(float3(0.10, 0.75, 0.60), float3(0.95, 0.85, 0.20), scaled - 2.0);
    }

    return lerp(float3(0.95, 0.85, 0.20), float3(0.95, 0.25, 0.10), saturate(scaled - 3.0));
}

float3 Diverging(float value)
{
    const float3 neutral = float3(0.96, 0.96, 0.96);

    return value >= 0.0 ? lerp(neutral, float3(0.85, 0.15, 0.10), saturate(value)) : lerp(neutral, float3(0.10, 0.30, 0.85), saturate(-value));
}

float3 LinearFromSRGB(float3 color)
{
    return pow(max(color, 0.0), 2.2);
}

#endif