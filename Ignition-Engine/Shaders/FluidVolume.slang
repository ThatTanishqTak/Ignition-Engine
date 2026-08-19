// The whole lattice in the viewport, not one plane of it: a fullscreen triangle, one ray per pixel, marched through the
// field with Beer-Lambert absorption. Colour comes from the same ramps the slice uses; opacity does not, because
// undisturbed freestream carries a large velocity everywhere and would fog the domain solid. Opacity keys off the
// departure from freestream instead, so clean air is exactly invisible and only what the body does to it shows up
#include "FluidColor.inc.slang"

struct VolumeParameters
{
    float4x4 InverseViewProjection;

    float3 LatticeMinimum;   // world position of cell (0,0,0)'s corner
    float CellSize;          // m per cell

    uint3 Resolution;
    uint Field;              // 0 velocity, 1 vorticity, 2 pressure

    float LatticeVelocity;   // freestream, lattice units
    float ColorScale;
    float Density;           // extinction per metre where the flow has departed fully from freestream
    float Threshold;         // departure below this fraction is perfectly clear air

    uint StepCount;
    uint Flags;              // bit 0: solid cells shade as the voxel mask instead of just stopping the ray
    uint Padding0;
    uint Padding1;
};

[[vk::push_constant]]
VolumeParameters g_Parameters;

[[vk::binding(0, 0)]] RWStructuredBuffer<uint> g_Flags;    // low byte: cell type, high byte: object id
[[vk::binding(1, 0)]] RWStructuredBuffer<float4> g_Fields; // (density, ux, uy, uz)

static const uint CellSolid = 1;
static const float MinimumTransmittance = 0.003; // below this the ray has nothing left to reveal

struct VSOut
{
    float4 Position : SV_Position;
    float2 NDC : TEXCOORD0;
};

// Three vertices cover the viewport with no vertex buffer and no diagonal seam
[shader("vertex")]
VSOut vertexMain(uint vertexID : SV_VertexID)
{
    const float2 corner = float2(float((vertexID << 1) & 2), float(vertexID & 2));

    VSOut output;
    output.NDC = corner * 2.0 - 1.0;
    output.Position = float4(output.NDC, 0.0, 1.0);

    return output;
}

// Same indexing as Fluid3D.slang: X runs fastest
uint CellIndex(uint3 coordinate)
{
    return (coordinate.z * g_Parameters.Resolution.y + coordinate.y) * g_Parameters.Resolution.x + coordinate.x;
}

bool IsSolid(uint flag)
{
    return (flag & 0xFFu) == CellSolid;
}

// A component of exactly zero makes (bound - origin) * inf a NaN on a face-aligned ray, so the reciprocal is floored
float3 SafeDirection(float3 direction)
{
    const float3 magnitude = max(abs(direction), float3(1e-6, 1e-6, 1e-6));

    return float3(
        direction.x < 0.0 ? -magnitude.x : magnitude.x,
        direction.y < 0.0 ? -magnitude.y : magnitude.y,
        direction.z < 0.0 ? -magnitude.z : magnitude.z);
}

// Trilinear over cell centres, exactly as the tracers sample it; solid cells hold zero velocity so walls decay naturally
float4 SampleField(float3 position)
{
    const float3 resolution = float3(g_Parameters.Resolution);
    const float3 gridPosition = clamp(position, float3(0.5, 0.5, 0.5), resolution - 0.5) - 0.5;
    const uint3 base = min(uint3(gridPosition), g_Parameters.Resolution - 2);
    const float3 fraction = gridPosition - float3(base);

    float4 value = float4(0.0, 0.0, 0.0, 0.0);

    for (uint corner = 0; corner < 8; ++corner)
    {
        const uint3 offset = uint3(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
        const float3 weights = lerp(1.0 - fraction, fraction, float3(offset));

        value += weights.x * weights.y * weights.z * g_Fields[CellIndex(base + offset)];
    }

    return value;
}

// Nearest-cell central differences: six extra loads a step, against eight for another trilinear fetch
float3 CellVorticity(uint3 cell)
{
    const uint3 resolution = g_Parameters.Resolution;
    const uint3 lower = uint3(max(int3(cell) - 1, int3(0, 0, 0)));
    const uint3 upper = min(cell + 1, resolution - 1);

    const float3 dx = (g_Fields[CellIndex(uint3(upper.x, cell.y, cell.z))].yzw - g_Fields[CellIndex(uint3(lower.x, cell.y, cell.z))].yzw) * 0.5;
    const float3 dy = (g_Fields[CellIndex(uint3(cell.x, upper.y, cell.z))].yzw - g_Fields[CellIndex(uint3(cell.x, lower.y, cell.z))].yzw) * 0.5;
    const float3 dz = (g_Fields[CellIndex(uint3(cell.x, cell.y, upper.z))].yzw - g_Fields[CellIndex(uint3(cell.x, cell.y, lower.z))].yzw) * 0.5;

    return float3(dy.z - dz.y, dz.x - dx.z, dx.y - dy.x);
}

[shader("fragment")]
float4 fragmentMain(VSOut input) : SV_Target
{
    // The near and far points of this pixel's ray, straight out of the inverse view-projection - no camera vector needed
    const float4 nearPoint = mul(g_Parameters.InverseViewProjection, float4(input.NDC, 0.0, 1.0));
    const float4 farPoint = mul(g_Parameters.InverseViewProjection, float4(input.NDC, 1.0, 1.0));

    const float3 origin = nearPoint.xyz / nearPoint.w;
    const float3 direction = normalize(farPoint.xyz / farPoint.w - origin);

    const float cellSize = max(g_Parameters.CellSize, 1e-6);
    const float3 boxMinimum = g_Parameters.LatticeMinimum;
    const float3 boxMaximum = boxMinimum + float3(g_Parameters.Resolution) * cellSize;

    const float3 inverseDirection = 1.0 / SafeDirection(direction);
    const float3 firstPlane = (boxMinimum - origin) * inverseDirection;
    const float3 secondPlane = (boxMaximum - origin) * inverseDirection;

    const float3 nearPlanes = min(firstPlane, secondPlane);
    const float3 farPlanes = max(firstPlane, secondPlane);

    // Clamping the entry at zero is what lets the camera sit inside the tunnel - which it always does, the tunnel is the scene
    const float entry = max(max(nearPlanes.x, nearPlanes.y), max(nearPlanes.z, 0.0));
    const float exit = min(min(farPlanes.x, farPlanes.y), farPlanes.z);

    if (exit <= entry)
    {
        discard;
    }

    const uint steps = clamp(g_Parameters.StepCount, 16u, 512u);
    const float stepSize = (exit - entry) / float(steps);

    const float reference = max(g_Parameters.LatticeVelocity * g_Parameters.ColorScale, 1e-6);
    const float pressureReference = max(0.02 * g_Parameters.ColorScale, 1e-6);
    const float freestream = max(g_Parameters.LatticeVelocity, 1e-6);
    const float threshold = saturate(g_Parameters.Threshold);

    float3 accumulated = float3(0.0, 0.0, 0.0);
    float transmittance = 1.0;

    for (uint step = 0; step < steps; ++step)
    {
        const float3 world = origin + direction * (entry + (float(step) + 0.5) * stepSize);
        const float3 lattice = (world - boxMinimum) / cellSize;
        const uint3 cell = uint3(clamp(lattice, float3(0.0, 0.0, 0.0), float3(g_Parameters.Resolution) - 1.0));

        const uint flag = g_Flags[CellIndex(cell)];

        if (IsSolid(flag))
        {
            if ((g_Parameters.Flags & 1u) != 0u)
            {
                // Voxel debug view: the mask shades itself, which is how a silhouette gets checked before a force is trusted
                const uint objectID = (flag >> 8) & 0xFFu;
                const float3 tint = objectID == 0 ? float3(0.55, 0.55, 0.60) : Ramp(frac(float(objectID) * 0.618034));

                accumulated += transmittance * LinearFromSRGB(tint);
                transmittance = 0.0;
            }

            // Every mesh is voxelized, so the mask is the scene geometry: stopping here is what makes a body hide the air behind it.
            // Transmittance is left alone when the mask is not being shaded, so the rasterized mesh shows through unharmed
            break;
        }

        const float4 field = SampleField(lattice);
        const float3 velocity = field.yzw;

        float value;     // colours the sample, on the scale the slice uses
        float departure; // opens it up, and reads zero in undisturbed freestream

        if (g_Parameters.Field == 0)
        {
            value = length(velocity) / (1.5 * reference);
            departure = abs(length(velocity) - freestream) / freestream;
        }
        else if (g_Parameters.Field == 1)
        {
            value = length(CellVorticity(cell)) / (0.5 * reference);
            departure = value;
        }
        else
        {
            value = (field.x - 1.0) / pressureReference;
            departure = abs(value);
        }

        const float extinction = g_Parameters.Density * saturate((departure - threshold) / max(1.0 - threshold, 1e-3));

        if (extinction <= 0.0)
        {
            continue;
        }

        // Beer-Lambert per metre, so raising the step count sharpens the picture instead of darkening it
        const float alpha = 1.0 - exp(-extinction * stepSize);
        const float3 color = g_Parameters.Field == 2 ? Diverging(value) : Ramp(value);

        accumulated += transmittance * alpha * LinearFromSRGB(color);
        transmittance *= 1.0 - alpha;

        if (transmittance < MinimumTransmittance)
        {
            break;
        }
    }

    const float opacity = 1.0 - transmittance;

    if (opacity <= 0.0)
    {
        discard;
    }

    // Premultiplied: the pipeline blends ONE / ONE_MINUS_SRC_ALPHA, so no dividing the colour back out
    return float4(accumulated, opacity);
}