static const int2 Directions[9] =
{
    int2( 0,  0),
    int2( 1,  0),
    int2( 0,  1),
    int2(-1,  0),
    int2( 0, -1),
    int2( 1,  1),
    int2(-1,  1),
    int2(-1, -1),
    int2( 1, -1),
};

static const float Weights[9] =
{
    4.0 / 9.0,
    1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0,
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
};

static const uint Opposite[9] = { 0, 3, 4, 1, 2, 7, 8, 5, 6 }; // bounce-back
static const uint Mirror[9] = { 0, 1, 4, 3, 2, 8, 7, 6, 5 };   // flips ey - free-slip walls

static const uint CellFluid = 0;
static const uint CellSolid = 1;

struct FluidParameters
{
    uint2 Resolution;
    float2 ObstacleCenter;      // lattice cells
    float ObstacleRadius;       // lattice cells
    float LatticeVelocity;
    float RelaxationTime;
    float SmagorinskyConstant;
    uint ReductionGroups;
    uint Field;
    float ColorScale;
    uint Padding;
};

[[vk::push_constant]]
FluidParameters g_Parameters;

[[vk::binding(0, 0)]] RWStructuredBuffer<float> g_Source;
[[vk::binding(1, 0)]] RWStructuredBuffer<float> g_Destination;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> g_Flags;
[[vk::binding(3, 0)]] RWStructuredBuffer<float4> g_Fields;      // (density, ux, uy, unused)
[[vk::binding(4, 0)]] RWStructuredBuffer<float2> g_CellForces;  // momentum exchange, per cell
[[vk::binding(5, 0)]] RWStructuredBuffer<float4> g_ForcePartials;
[[vk::binding(6, 0)]] RWStructuredBuffer<float4> g_ForceResult;

// An explicit format keeps this off the shaderStorageImageWriteWithoutFormat feature
[[vk::binding(7, 0)]]
[format("rgba8")]
RWTexture2D<float4> g_Visualization;

uint CellCount()
{
    return g_Parameters.Resolution.x * g_Parameters.Resolution.y;
}

uint CellIndex(uint x, uint y)
{
    return y * g_Parameters.Resolution.x + x;
}

float Equilibrium(uint direction, float density, float2 velocity)
{
    const float projection = dot(float2(Directions[direction]), velocity);
    const float speedSquared = dot(velocity, velocity);

    return Weights[direction] * density * (1.0 + 3.0 * projection + 4.5 * projection * projection - 1.5 * speedSquared);
}

void WriteEquilibrium(uint cell, uint count, float density, float2 velocity)
{
    for (uint direction = 0; direction < 9; ++direction)
    {
        g_Destination[direction * count + cell] = Equilibrium(direction, density, velocity);
    }

    g_Fields[cell] = float4(density, velocity, 0.0);
}

[shader("compute")]
[numthreads(8, 8, 1)]
void initialize(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_Parameters.Resolution.x || id.y >= g_Parameters.Resolution.y)
    {
        return;
    }

    const uint count = CellCount();
    const uint cell = CellIndex(id.x, id.y);

    const float2 position = float2(float(id.x) + 0.5, float(id.y) + 0.5);
    const bool solid = distance(position, g_Parameters.ObstacleCenter) <= g_Parameters.ObstacleRadius;

    g_Flags[cell] = solid ? CellSolid : CellFluid;
    g_CellForces[cell] = float2(0.0, 0.0);

    if (solid)
    {
        for (uint direction = 0; direction < 9; ++direction)
        {
            g_Destination[direction * count + cell] = 0.0;
        }

        g_Fields[cell] = float4(1.0, 0.0, 0.0, 0.0);

        return;
    }

    WriteEquilibrium(cell, count, 1.0, float2(g_Parameters.LatticeVelocity, 0.0));
}

[shader("compute")]
[numthreads(8, 8, 1)]
void streamCollide(uint3 id : SV_DispatchThreadID)
{
    const uint width = g_Parameters.Resolution.x;
    const uint height = g_Parameters.Resolution.y;

    if (id.x >= width || id.y >= height)
    {
        return;
    }

    const uint count = CellCount();
    const uint cell = CellIndex(id.x, id.y);

    g_CellForces[cell] = float2(0.0, 0.0);

    if (g_Flags[cell] == CellSolid)
    {
        for (uint direction = 0; direction < 9; ++direction)
        {
            g_Destination[direction * count + cell] = 0.0;
        }

        g_Fields[cell] = float4(1.0, 0.0, 0.0, 0.0);

        return;
    }

    // Inlet: equilibrium at the test speed
    if (id.x == 0)
    {
        WriteEquilibrium(cell, count, 1.0, float2(g_Parameters.LatticeVelocity, 0.0));

        return;
    }

    // Outlet: zero gradient, copied from the column upstream
    if (id.x == width - 1)
    {
        const uint neighbour = CellIndex(width - 2, id.y);

        float density = 0.0;
        float2 momentum = float2(0.0, 0.0);

        for (uint direction = 0; direction < 9; ++direction)
        {
            const float value = g_Source[direction * count + neighbour];

            g_Destination[direction * count + cell] = value;

            density += value;
            momentum += value * float2(Directions[direction]);
        }

        g_Fields[cell] = float4(density, momentum / max(density, 1e-6), 0.0);

        return;
    }

    // Pull scheme: gather each population from the neighbour it came from
    float distribution[9];
    float2 force = float2(0.0, 0.0);

    for (uint direction = 0; direction < 9; ++direction)
    {
        const int sourceX = int(id.x) - Directions[direction].x;
        const int sourceY = int(id.y) - Directions[direction].y;

        if (sourceY < 0 || sourceY >= int(height))
        {
            // Free-slip top/bottom: specular reflection keeps the wall from growing a boundary layer
            distribution[direction] = g_Source[Mirror[direction] * count + cell];

            continue;
        }

        const uint source = CellIndex(uint(sourceX), uint(sourceY));

        if (g_Flags[source] == CellSolid)
        {
            // Half-way bounce-back. The link points from this cell toward the solid along Opposite[direction], so Ladd momentum exchange contributes 2 * e * f along that link.
            const uint towardsSolid = Opposite[direction];
            const float outgoing = g_Source[towardsSolid * count + cell];

            distribution[direction] = outgoing;
            force += 2.0 * float2(Directions[towardsSolid]) * outgoing;
        }
        else
        {
            distribution[direction] = g_Source[direction * count + source];
        }
    }

    float density = 0.0;
    float2 momentum = float2(0.0, 0.0);

    for (uint moment = 0; moment < 9; ++moment)
    {
        density += distribution[moment];
        momentum += distribution[moment] * float2(Directions[moment]);
    }

    const float2 velocity = momentum / max(density, 1e-6);

    float equilibrium[9];
    float stressXX = 0.0;
    float stressXY = 0.0;
    float stressYY = 0.0;

    for (uint stress = 0; stress < 9; ++stress)
    {
        equilibrium[stress] = Equilibrium(stress, density, velocity);

        const float nonEquilibrium = distribution[stress] - equilibrium[stress];
        const float2 direction = float2(Directions[stress]);

        stressXX += direction.x * direction.x * nonEquilibrium;
        stressXY += direction.x * direction.y * nonEquilibrium;
        stressYY += direction.y * direction.y * nonEquilibrium;
    }

    // Smagorinsky subgrid viscosity: without it the solver blows up well before the interesting Reynolds numbers
    const float stressNorm = sqrt(stressXX * stressXX + 2.0 * stressXY * stressXY + stressYY * stressYY);
    const float baseTau = g_Parameters.RelaxationTime;
    const float constantSquared = g_Parameters.SmagorinskyConstant * g_Parameters.SmagorinskyConstant;
    const float tau = 0.5 * (baseTau + sqrt(baseTau * baseTau + 18.0 * 1.4142135 * constantSquared * stressNorm / max(density, 1e-6)));
    const float omega = clamp(1.0 / tau, 0.0, 1.99);

    for (uint collide = 0; collide < 9; ++collide)
    {
        g_Destination[collide * count + cell] = distribution[collide] - omega * (distribution[collide] - equilibrium[collide]);
    }

    g_Fields[cell] = float4(density, velocity, 0.0);
    g_CellForces[cell] = force;
}

groupshared float2 s_ForcePartial[256];

void ReduceGroup(uint groupIndex)
{
    for (uint offset = 128; offset > 0; offset >>= 1)
    {
        if (groupIndex < offset)
        {
            s_ForcePartial[groupIndex] += s_ForcePartial[groupIndex + offset];
        }

        GroupMemoryBarrierWithGroupSync();
    }
}

[shader("compute")]
[numthreads(256, 1, 1)]
void reduceForcePartial(uint3 dispatchID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint count = CellCount();
    const uint stride = max(g_Parameters.ReductionGroups, 1u) * 256;

    float2 sum = float2(0.0, 0.0);

    for (uint index = dispatchID.x; index < count; index += stride)
    {
        sum += g_CellForces[index];
    }

    s_ForcePartial[groupIndex] = sum;
    GroupMemoryBarrierWithGroupSync();

    ReduceGroup(groupIndex);

    if (groupIndex == 0)
    {
        g_ForcePartials[groupID.x] = float4(s_ForcePartial[0], 0.0, 0.0);
    }
}

[shader("compute")]
[numthreads(256, 1, 1)]
void reduceForceFinal(uint groupIndex : SV_GroupIndex)
{
    const uint groups = max(g_Parameters.ReductionGroups, 1u);

    float2 sum = float2(0.0, 0.0);

    for (uint index = groupIndex; index < groups; index += 256)
    {
        sum += g_ForcePartials[index].xy;
    }

    s_ForcePartial[groupIndex] = sum;
    GroupMemoryBarrierWithGroupSync();

    ReduceGroup(groupIndex);

    if (groupIndex == 0)
    {
        g_ForceResult[0] = float4(s_ForcePartial[0], 0.0, 0.0);
    }
}

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

[shader("compute")]
[numthreads(8, 8, 1)]
void visualize(uint3 id : SV_DispatchThreadID)
{
    const uint width = g_Parameters.Resolution.x;
    const uint height = g_Parameters.Resolution.y;

    if (id.x >= width || id.y >= height)
    {
        return;
    }

    const uint cell = CellIndex(id.x, id.y);

    float3 color;

    if (g_Flags[cell] == CellSolid)
    {
        // The mask is drawn as part of the field so boundary bugs are visible instead of inferred
        color = float3(0.12, 0.12, 0.14);
    }
    else
    {
        const float4 field = g_Fields[cell];
        const float reference = max(g_Parameters.LatticeVelocity * g_Parameters.ColorScale, 1e-6);

        if (g_Parameters.Field == 0)
        {
            color = Ramp(length(field.yz) / (1.5 * reference));
        }
        else if (g_Parameters.Field == 1)
        {
            float vorticity = 0.0;

            if (id.x > 0 && id.x + 1 < width && id.y > 0 && id.y + 1 < height)
            {
                vorticity = 0.5 * (g_Fields[CellIndex(id.x + 1, id.y)].z - g_Fields[CellIndex(id.x - 1, id.y)].z) - 0.5 * (g_Fields[CellIndex(id.x, id.y + 1)].y - g_Fields[CellIndex(id.x, id.y - 1)].y);
            }

            color = Diverging(vorticity / (0.5 * reference));
        }
        else
        {
            color = Diverging((field.x - 1.0) / max(0.02 * g_Parameters.ColorScale, 1e-6));
        }
    }

    // The composite pass writes into an sRGB swapchain image, so the texture holds linear values
    g_Visualization[uint2(id.x, height - 1 - id.y)] = float4(LinearFromSRGB(color), 1.0);
}