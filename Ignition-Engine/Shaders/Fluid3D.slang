static const int3 Directions[19] =
{
    int3( 0,  0,  0),
    int3( 1,  0,  0), int3(-1,  0,  0),
    int3( 0,  1,  0), int3( 0, -1,  0),
    int3( 0,  0,  1), int3( 0,  0, -1),
    int3( 1,  1,  0), int3(-1, -1,  0),
    int3( 1, -1,  0), int3(-1,  1,  0),
    int3( 1,  0,  1), int3(-1,  0, -1),
    int3( 1,  0, -1), int3(-1,  0,  1),
    int3( 0,  1,  1), int3( 0, -1, -1),
    int3( 0,  1, -1), int3( 0, -1,  1),
};

static const float Weights[19] =
{
    1.0 / 3.0,
    1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0, 1.0 / 18.0,
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
    1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0, 1.0 / 36.0,
};

static const uint Opposite[19] = { 0, 2, 1, 4, 3, 6, 5, 8, 7, 10, 9, 12, 11, 14, 13, 16, 15, 18, 17 }; // bounce-back
static const uint MirrorX[19]  = { 0, 2, 1, 3, 4, 5, 6, 10, 9, 8, 7, 14, 13, 12, 11, 15, 16, 17, 18 }; // flips ex - free-slip side walls
static const uint MirrorY[19]  = { 0, 1, 2, 4, 3, 5, 6, 9, 10, 7, 8, 11, 12, 13, 14, 18, 17, 16, 15 }; // flips ey - free-slip roof

static const uint CellFluid = 0;
static const uint CellSolid = 1;

struct FluidParameters3D
{
    uint3 Resolution;
    uint ReductionGroups;
    float3 ObstacleCenter;      // lattice cells
    float ObstacleRadius;       // lattice cells
    float3 ReferencePoint;      // lattice cells, torque is taken about this
    float LatticeVelocity;
    float RelaxationTime;
    float SmagorinskyConstant;
    uint Flags;                 // bit 0: rolling road
    uint Padding;

    // Visualization (Step 3.4)
    uint SliceAxis;
    uint SliceField;
    float SliceFraction;
    float ColorScale;
    uint ParticleCount;
    uint FrameSeed;
    uint ShellVertexCapacity;
    float AdvectSteps;          // lattice steps this frame - zero while paused, which freezes the tracers
};

[[vk::push_constant]]
FluidParameters3D g_Parameters;

[[vk::binding(0, 0)]] RWStructuredBuffer<float> g_Source;
[[vk::binding(1, 0)]] RWStructuredBuffer<float> g_Destination;
[[vk::binding(2, 0)]] RWStructuredBuffer<uint> g_Flags;         // low byte: cell type. high byte: object id, so Phase 4 can turn wheels without touching the format
[[vk::binding(3, 0)]] RWStructuredBuffer<float4> g_Fields;      // (density, ux, uy, uz)
[[vk::binding(4, 0)]] RWStructuredBuffer<float> g_CellForces;   // momentum exchange, SoA: component * cellCount + cell
[[vk::binding(5, 0)]] RWStructuredBuffer<float> g_ForcePartials; // six components: force then torque
[[vk::binding(6, 0)]] RWStructuredBuffer<float> g_ForceResult;
[[vk::binding(7, 0)]] RWStructuredBuffer<uint> g_Area;          // inlet-facing solid columns

// Visualization resources (Step 3.4)
[[vk::binding(8, 0)]] RWStructuredBuffer<float4> g_Particles;        // xyz position (lattice), w remaining life in steps
[[vk::binding(9, 0)]] RWStructuredBuffer<float> g_ParticleVertices;  // DebugLineVertex layout: 7 floats per vertex, 2 vertices per particle
[[vk::binding(10, 0)]] RWStructuredBuffer<float> g_ShellVertices;    // same layout, appended by the surface shell
[[vk::binding(11, 0)]] RWStructuredBuffer<uint> g_ShellIndirect;     // VkDrawIndirectCommand: vertexCount, instanceCount, firstVertex, firstInstance

// An explicit format keeps this off the shaderStorageImageWriteWithoutFormat feature, like the 2D lab's image
[[vk::binding(12, 0)]]
[format("rgba8")]
RWTexture2D<float4> g_Slice;

static const uint ParticleCapacity = 262144; // must match VulkanFluidSolver3D.cpp
static const uint SliceImageSize = 512;      // must match VulkanFluidSolver3D.cpp
static const float TrailLength = 6.0;        // lattice cells of streak per unit lattice velocity - keeps direction visible while paused

uint CellCount()
{
    return g_Parameters.Resolution.x * g_Parameters.Resolution.y * g_Parameters.Resolution.z;
}

// X runs fastest so a workgroup's threads read contiguous memory
uint CellIndex(uint3 coordinate)
{
    return (coordinate.z * g_Parameters.Resolution.y + coordinate.y) * g_Parameters.Resolution.x + coordinate.x;
}

bool IsSolid(uint flag)
{
    return (flag & 0xFFu) == CellSolid;
}

uint MakeFlag(uint type, uint objectID)
{
    return type | (objectID << 8);
}

// Air flows in -Z: the car faces +Z and the inlet is the +Z face
float3 InletVelocity()
{
    return float3(0.0, 0.0, -g_Parameters.LatticeVelocity);
}

float Equilibrium(uint direction, float density, float3 velocity)
{
    const float projection = dot(float3(Directions[direction]), velocity);
    const float speedSquared = dot(velocity, velocity);

    return Weights[direction] * density * (1.0 + 3.0 * projection + 4.5 * projection * projection - 1.5 * speedSquared);
}

void WriteEquilibrium(uint cell, uint count, float density, float3 velocity)
{
    for (uint direction = 0; direction < 19; ++direction)
    {
        g_Destination[direction * count + cell] = Equilibrium(direction, density, velocity);
    }

    g_Fields[cell] = float4(density, velocity);
}

void WriteSolid(uint cell, uint count)
{
    for (uint direction = 0; direction < 19; ++direction)
    {
        g_Destination[direction * count + cell] = 0.0;
    }

    g_Fields[cell] = float4(1.0, 0.0, 0.0, 0.0);
}

[shader("compute")]
[numthreads(8, 8, 4)]
void initialize(uint3 id : SV_DispatchThreadID)
{
    if (any(id >= g_Parameters.Resolution))
    {
        return;
    }

    const uint count = CellCount();
    const uint cell = CellIndex(id);

    if (cell == 0)
    {
        // Cleared here rather than in its own dispatch; projectedArea runs after a barrier and is the only other writer
        g_Area[0] = 0;
    }

    // Analytic sphere until the voxelizer writes this same mask from a mesh - the format is what the two share, not the code
    const float3 position = float3(id) + 0.5;
    const bool solid = distance(position, g_Parameters.ObstacleCenter) <= g_Parameters.ObstacleRadius;

    g_Flags[cell] = solid ? MakeFlag(CellSolid, 1u) : MakeFlag(CellFluid, 0u);

    for (uint axis = 0; axis < 3; ++axis)
    {
        g_CellForces[axis * count + cell] = 0.0;
    }

    if (solid)
    {
        WriteSolid(cell, count);

        return;
    }

    WriteEquilibrium(cell, count, 1.0, InletVelocity());
}

[shader("compute")]
[numthreads(8, 8, 1)]
void projectedArea(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_Parameters.Resolution.x || id.y >= g_Parameters.Resolution.y)
    {
        return;
    }

    // Frontal area is the silhouette the wind sees: one count per column that contains any solid at all
    for (uint z = 0; z < g_Parameters.Resolution.z; ++z)
    {
        if (IsSolid(g_Flags[CellIndex(uint3(id.x, id.y, z))]))
        {
            InterlockedAdd(g_Area[0], 1u);

            return;
        }
    }
}

[shader("compute")]
[numthreads(8, 8, 4)]
void streamCollide(uint3 id : SV_DispatchThreadID)
{
    const uint3 resolution = g_Parameters.Resolution;

    if (any(id >= resolution))
    {
        return;
    }

    const uint count = CellCount();
    const uint cell = CellIndex(id);

    for (uint clear = 0; clear < 3; ++clear)
    {
        g_CellForces[clear * count + cell] = 0.0;
    }

    if (IsSolid(g_Flags[cell]))
    {
        WriteSolid(cell, count);

        return;
    }

    const float3 inlet = InletVelocity();

    // Inlet: equilibrium at the test speed, on the +Z face
    if (id.z == resolution.z - 1)
    {
        WriteEquilibrium(cell, count, 1.0, inlet);

        return;
    }

    // Outlet: zero gradient, copied from the plane upstream
    if (id.z == 0)
    {
        const uint neighbour = CellIndex(uint3(id.x, id.y, 1));

        float outletDensity = 0.0;
        float3 outletMomentum = float3(0.0, 0.0, 0.0);

        for (uint direction = 0; direction < 19; ++direction)
        {
            const float value = g_Source[direction * count + neighbour];

            g_Destination[direction * count + cell] = value;

            outletDensity += value;
            outletMomentum += value * float3(Directions[direction]);
        }

        g_Fields[cell] = float4(outletDensity, outletMomentum / max(outletDensity, 1e-6));

        return;
    }

    // A static floor grows a boundary layer that visibly corrupts underbody flow, which is where an F1 car makes its downforce. The road runs at the wind speed instead
    const float3 wallVelocity = (g_Parameters.Flags & 1u) != 0u ? inlet : float3(0.0, 0.0, 0.0);

    // Pull scheme: gather each population from the neighbour it came from
    float distribution[19];
    float3 force = float3(0.0, 0.0, 0.0);

    for (uint direction = 0; direction < 19; ++direction)
    {
        const int3 source = int3(id) - Directions[direction];

        if (source.y < 0)
        {
            // Rolling road: half-way bounce-back with the wall's momentum added back in. Density is taken as 1 - the incompressible approximation the rest of the kernel already makes
            const uint towardsWall = Opposite[direction];

            distribution[direction] = g_Source[towardsWall * count + cell] + 6.0 * Weights[direction] * dot(float3(Directions[direction]), wallVelocity);

            continue;
        }

        if (source.y >= int(resolution.y))
        {
            // Free-slip roof: specular reflection, so the ceiling never grows a boundary layer of its own
            distribution[direction] = g_Source[MirrorY[direction] * count + cell];

            continue;
        }

        if (source.x < 0 || source.x >= int(resolution.x))
        {
            distribution[direction] = g_Source[MirrorX[direction] * count + cell];

            continue;
        }

        const uint neighbour = CellIndex(uint3(source));

        if (IsSolid(g_Flags[neighbour]))
        {
            const uint towardsSolid = Opposite[direction];
            const float outgoing = g_Source[towardsSolid * count + cell];
            const float reflected = outgoing;

            distribution[direction] = reflected;
            force += float3(Directions[towardsSolid]) * (outgoing + reflected);

            continue;
        }

        distribution[direction] = g_Source[direction * count + neighbour];
    }

    float density = 0.0;
    float3 momentum = float3(0.0, 0.0, 0.0);

    for (uint moment = 0; moment < 19; ++moment)
    {
        density += distribution[moment];
        momentum += distribution[moment] * float3(Directions[moment]);
    }

    const float3 velocity = momentum / max(density, 1e-6);

    float equilibrium[19];
    float stressXX = 0.0;
    float stressYY = 0.0;
    float stressZZ = 0.0;
    float stressXY = 0.0;
    float stressXZ = 0.0;
    float stressYZ = 0.0;

    for (uint stress = 0; stress < 19; ++stress)
    {
        equilibrium[stress] = Equilibrium(stress, density, velocity);

        const float nonEquilibrium = distribution[stress] - equilibrium[stress];
        const float3 direction = float3(Directions[stress]);

        stressXX += direction.x * direction.x * nonEquilibrium;
        stressYY += direction.y * direction.y * nonEquilibrium;
        stressZZ += direction.z * direction.z * nonEquilibrium;
        stressXY += direction.x * direction.y * nonEquilibrium;
        stressXZ += direction.x * direction.z * nonEquilibrium;
        stressYZ += direction.y * direction.z * nonEquilibrium;
    }

    // Smagorinsky subgrid viscosity. In 3D it is doing more than keeping the solver alive: with air's real viscosity the resolved tau sits on 0.5, so this is what supplies the effective viscosity
    const float stressNorm = sqrt(stressXX * stressXX + stressYY * stressYY + stressZZ * stressZZ + 2.0 * (stressXY * stressXY + stressXZ * stressXZ + stressYZ * stressYZ));
    const float baseTau = g_Parameters.RelaxationTime;
    const float constantSquared = g_Parameters.SmagorinskyConstant * g_Parameters.SmagorinskyConstant;
    const float tau = 0.5 * (baseTau + sqrt(baseTau * baseTau + 18.0 * 1.4142135 * constantSquared * stressNorm / max(density, 1e-6)));
    const float omega = clamp(1.0 / tau, 0.0, 1.99);

    for (uint collide = 0; collide < 19; ++collide)
    {
        g_Destination[collide * count + cell] = distribution[collide] - omega * (distribution[collide] - equilibrium[collide]);
    }

    g_Fields[cell] = float4(density, velocity);

    for (uint axis = 0; axis < 3; ++axis)
    {
        g_CellForces[axis * count + cell] = force[axis];
    }
}

groupshared float3 s_Force[256];
groupshared float3 s_Torque[256];

void ReduceGroup(uint groupIndex)
{
    for (uint offset = 128; offset > 0; offset >>= 1)
    {
        if (groupIndex < offset)
        {
            s_Force[groupIndex] += s_Force[groupIndex + offset];
            s_Torque[groupIndex] += s_Torque[groupIndex + offset];
        }

        GroupMemoryBarrierWithGroupSync();
    }
}

[shader("compute")]
[numthreads(256, 1, 1)]
void reduceForcePartial(uint3 dispatchID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    const uint3 resolution = g_Parameters.Resolution;
    const uint count = CellCount();
    const uint groups = max(g_Parameters.ReductionGroups, 1u);
    const uint stride = groups * 256;

    float3 force = float3(0.0, 0.0, 0.0);
    float3 torque = float3(0.0, 0.0, 0.0);

    for (uint index = dispatchID.x; index < count; index += stride)
    {
        const float3 cellForce = float3(g_CellForces[index], g_CellForces[count + index], g_CellForces[2 * count + index]);

        // The moment arm is the cell's own position, so only the force needs storing per cell - that is 12 bytes a cell instead of 24
        const uint3 coordinate = uint3(index % resolution.x, (index / resolution.x) % resolution.y, index / (resolution.x * resolution.y));
        const float3 arm = float3(coordinate) + 0.5 - g_Parameters.ReferencePoint;

        force += cellForce;
        torque += cross(arm, cellForce);
    }

    s_Force[groupIndex] = force;
    s_Torque[groupIndex] = torque;
    GroupMemoryBarrierWithGroupSync();

    ReduceGroup(groupIndex);

    if (groupIndex == 0)
    {
        g_ForceResult[0] = s_Force[0].x;
        g_ForceResult[1] = s_Force[0].y;
        g_ForceResult[2] = s_Force[0].z;
        g_ForceResult[3] = s_Torque[0].x;
        g_ForceResult[4] = s_Torque[0].y;
        g_ForceResult[5] = s_Torque[0].z;
    }
}

// ---------------- Step 3.4: visualization ----------------
// The 2D lab's viz kernel is the template: same color maps, same linear-values-into-an-sRGB-composite convention

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

uint Hash(uint value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;

    return value;
}

float HashFloat(uint value)
{
    return float(Hash(value) & 0xFFFFFFu) / 16777216.0;
}

// Trilinear over cell centres; solid cells hold zero velocity, so the interpolation decays naturally at walls
float3 SampleVelocity(float3 position)
{
    const float3 resolution = float3(g_Parameters.Resolution);
    const float3 gridPosition = clamp(position, float3(0.5, 0.5, 0.5), resolution - 0.5) - 0.5;
    const uint3 base = min(uint3(gridPosition), g_Parameters.Resolution - 2);
    const float3 fraction = gridPosition - float3(base);

    float3 velocity = float3(0.0, 0.0, 0.0);

    for (uint corner = 0; corner < 8; ++corner)
    {
        const uint3 offset = uint3(corner & 1, (corner >> 1) & 1, (corner >> 2) & 1);
        const float3 weights = lerp(1.0 - fraction, fraction, float3(offset));

        velocity += weights.x * weights.y * weights.z * g_Fields[CellIndex(base + offset)].yzw;
    }

    return velocity;
}

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

void StoreParticleVertex(uint vertex, float3 position, float4 color)
{
    const uint base = vertex * 7;

    g_ParticleVertices[base + 0] = position.x;
    g_ParticleVertices[base + 1] = position.y;
    g_ParticleVertices[base + 2] = position.z;
    g_ParticleVertices[base + 3] = color.r;
    g_ParticleVertices[base + 4] = color.g;
    g_ParticleVertices[base + 5] = color.b;
    g_ParticleVertices[base + 6] = color.a;
}

void StoreShellVertex(uint vertex, float3 position, float4 color)
{
    const uint base = vertex * 7;

    g_ShellVertices[base + 0] = position.x;
    g_ShellVertices[base + 1] = position.y;
    g_ShellVertices[base + 2] = position.z;
    g_ShellVertices[base + 3] = color.r;
    g_ShellVertices[base + 4] = color.g;
    g_ShellVertices[base + 5] = color.b;
    g_ShellVertices[base + 6] = color.a;
}

// One pixel of the slice image maps to a continuous position on the plane, so the whole texture is always valid and UV 0-1 on the quad is always exact, whichever axis is chosen
[shader("compute")]
[numthreads(8, 8, 1)]
void sliceVisualize(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= SliceImageSize || id.y >= SliceImageSize)
    {
        return;
    }

    const float3 resolution = float3(g_Parameters.Resolution);
    const float u = (float(id.x) + 0.5) / float(SliceImageSize);
    const float v = (float(id.y) + 0.5) / float(SliceImageSize);

    float3 position;

    if (g_Parameters.SliceAxis == 0)
    {
        position = float3(g_Parameters.SliceFraction * resolution.x, v * resolution.y, u * resolution.z);
    }
    else if (g_Parameters.SliceAxis == 1)
    {
        position = float3(u * resolution.x, g_Parameters.SliceFraction * resolution.y, v * resolution.z);
    }
    else
    {
        position = float3(u * resolution.x, v * resolution.y, g_Parameters.SliceFraction * resolution.z);
    }

    const uint3 cell = uint3(clamp(position, float3(0.0, 0.0, 0.0), resolution - 1.0));
    const uint index = CellIndex(cell);

    float3 color;

    if (IsSolid(g_Flags[index]))
    {
        // The mask is drawn as part of the field so voxelizer bugs will be visible instead of inferred
        color = float3(0.12, 0.12, 0.14);
    }
    else
    {
        const float4 field = g_Fields[index];
        const float reference = max(g_Parameters.LatticeVelocity * g_Parameters.ColorScale, 1e-6);

        if (g_Parameters.SliceField == 0)
        {
            color = Ramp(length(field.yzw) / (1.5 * reference));
        }
        else if (g_Parameters.SliceField == 1)
        {
            color = Ramp(length(CellVorticity(cell)) / (0.5 * reference));
        }
        else
        {
            color = Diverging((field.x - 1.0) / max(0.02 * g_Parameters.ColorScale, 1e-6));
        }
    }

    // Row 0 holds the top of the plane, so ImGui shows it upright; the scene quad's UVs account for the same flip
    g_Slice[uint2(id.x, SliceImageSize - 1 - id.y)] = float4(LinearFromSRGB(color), 1.0);
}

[shader("compute")]
[numthreads(256, 1, 1)]
void initializeParticles(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= ParticleCapacity)
    {
        return;
    }

    const float3 resolution = float3(g_Parameters.Resolution);
    const uint seed = id.x * 747796405u + 2891336453u;

    // Seeded through the whole volume so the tunnel reads as full of air immediately; anything inside the body respawns on its first advection
    const float3 position = float3(
        1.0 + HashFloat(seed) * (resolution.x - 2.0),
        1.0 + HashFloat(seed + 1u) * (resolution.y - 2.0),
        1.0 + HashFloat(seed + 2u) * (resolution.z - 2.0));

    g_Particles[id.x] = float4(position, 100.0 + 900.0 * HashFloat(seed + 3u));

    // Degenerate, fully transparent segments: drawing before the first advection is harmless
    StoreParticleVertex(id.x * 2 + 0, position, float4(0.0, 0.0, 0.0, 0.0));
    StoreParticleVertex(id.x * 2 + 1, position, float4(0.0, 0.0, 0.0, 0.0));
}

[shader("compute")]
[numthreads(256, 1, 1)]
void advectParticles(uint3 id : SV_DispatchThreadID)
{
    if (id.x >= g_Parameters.ParticleCount || id.x >= ParticleCapacity)
    {
        return;
    }

    const float3 resolution = float3(g_Parameters.Resolution);

    float4 particle = g_Particles[id.x];
    float3 position = particle.xyz;
    float life = particle.w;

    // RK2: sample, step to the midpoint, sample again
    const float3 k1 = SampleVelocity(position);
    const float3 k2 = SampleVelocity(position + 0.5 * g_Parameters.AdvectSteps * k1);

    position += g_Parameters.AdvectSteps * k2;
    life -= g_Parameters.AdvectSteps;

    const bool outside = any(position < float3(1.0, 1.0, 1.0)) || any(position > resolution - 1.0);
    const bool inSolid = IsSolid(g_Flags[CellIndex(uint3(clamp(position, float3(0.0, 0.0, 0.0), resolution - 1.0)))]);

    if (outside || inSolid || life <= 0.0)
    {
        // Respawn in a thin band at the inlet, jittered per particle per frame. Life-based respawn keeps dead zones from hoarding stagnant tracers
        const uint seed = id.x * 747796405u + g_Parameters.FrameSeed * 2891336453u;

        position = float3(
            1.0 + HashFloat(seed) * (resolution.x - 2.0),
            1.0 + HashFloat(seed + 1u) * (resolution.y - 2.0),
            resolution.z - 2.0 - 2.0 * HashFloat(seed + 2u));
        life = 300.0 + 900.0 * HashFloat(seed + 3u);

        g_Particles[id.x] = float4(position, life);

        // A degenerate segment instead of a streak across the whole domain
        StoreParticleVertex(id.x * 2 + 0, position, float4(0.0, 0.0, 0.0, 0.0));
        StoreParticleVertex(id.x * 2 + 1, position, float4(0.0, 0.0, 0.0, 0.0));

        return;
    }

    g_Particles[id.x] = float4(position, life);

    const float3 velocity = SampleVelocity(position);
    const float reference = max(g_Parameters.LatticeVelocity * g_Parameters.ColorScale, 1e-6);
    const float3 color = LinearFromSRGB(Ramp(length(velocity) / (1.5 * reference)));

    // The tail fades to transparent, so the streak reads as motion even when the sim is paused
    const float3 tail = position - velocity * TrailLength;

    StoreParticleVertex(id.x * 2 + 0, tail, float4(color, 0.0));
    StoreParticleVertex(id.x * 2 + 1, position, float4(color, 0.9));
}

// Surface voxels are solid cells with at least one fluid axis-neighbour; near-wall density is the LBM's pressure
[shader("compute")]
[numthreads(8, 8, 4)]
void surfaceShell(uint3 id : SV_DispatchThreadID)
{
    if (any(id >= g_Parameters.Resolution))
    {
        return;
    }

    const uint cell = CellIndex(id);

    if (!IsSolid(g_Flags[cell]))
    {
        return;
    }

    float density = 0.0;
    uint fluidNeighbours = 0;

    for (uint direction = 1; direction < 7; ++direction) // Directions[1..6] are the six axis neighbours
    {
        const int3 neighbour = int3(id) + Directions[direction];

        if (any(neighbour < int3(0, 0, 0)) || any(neighbour >= int3(g_Parameters.Resolution)))
        {
            continue;
        }

        const uint neighbourCell = CellIndex(uint3(neighbour));

        if (!IsSolid(g_Flags[neighbourCell]))
        {
            density += g_Fields[neighbourCell].x;
            ++fluidNeighbours;
        }
    }

    if (fluidNeighbours == 0)
    {
        return; // interior solid - the flood fill's territory, nothing to see
    }

    const float pressure = density / float(fluidNeighbours) - 1.0;
    const float4 color = float4(LinearFromSRGB(Diverging(pressure / max(0.02 * g_Parameters.ColorScale, 1e-6))), 1.0);

    uint slot;
    InterlockedAdd(g_ShellIndirect[0], 6, slot);

    if (slot + 6 > g_Parameters.ShellVertexCapacity)
    {
        return; // the finalize kernel clamps the count back to what actually fits
    }

    const float3 center = float3(id) + 0.5;
    const float halfSize = 0.45;

    StoreShellVertex(slot + 0, center - float3(halfSize, 0.0, 0.0), color);
    StoreShellVertex(slot + 1, center + float3(halfSize, 0.0, 0.0), color);
    StoreShellVertex(slot + 2, center - float3(0.0, halfSize, 0.0), color);
    StoreShellVertex(slot + 3, center + float3(0.0, halfSize, 0.0), color);
    StoreShellVertex(slot + 4, center - float3(0.0, 0.0, halfSize), color);
    StoreShellVertex(slot + 5, center + float3(0.0, 0.0, halfSize), color);
}

[shader("compute")]
[numthreads(1, 1, 1)]
void shellFinalize(uint3 id : SV_DispatchThreadID)
{
    // Clamp after the append pass so an overflowing frame draws a full buffer instead of reading past it
    g_ShellIndirect[0] = min(g_ShellIndirect[0], g_Parameters.ShellVertexCapacity);
}

[shader("compute")]
[numthreads(256, 1, 1)]
void reduceForceFinal(uint groupIndex : SV_GroupIndex)
{
    const uint groups = max(g_Parameters.ReductionGroups, 1u);

    float3 force = float3(0.0, 0.0, 0.0);
    float3 torque = float3(0.0, 0.0, 0.0);

    for (uint index = groupIndex; index < groups; index += 256)
    {
        force += float3(g_ForcePartials[index], g_ForcePartials[groups + index], g_ForcePartials[2 * groups + index]);
        torque += float3(g_ForcePartials[3 * groups + index], g_ForcePartials[4 * groups + index], g_ForcePartials[5 * groups + index]);
    }

    s_Force[groupIndex] = force;
    s_Torque[groupIndex] = torque;
    GroupMemoryBarrierWithGroupSync();

    ReduceGroup(groupIndex);

    if (groupIndex == 0)
    {
        g_ForceResult[0] = s_Force[0].x;
        g_ForceResult[1] = s_Force[0].y;
        g_ForceResult[2] = s_Force[0].z;
        g_ForceResult[3] = s_Torque[0].x;
        g_ForceResult[4] = s_Torque[0].y;
        g_ForceResult[5] = s_Torque[0].z;
    }
}