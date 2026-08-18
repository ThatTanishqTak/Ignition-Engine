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
        g_ForcePartials[0 * groups + groupID.x] = s_Force[0].x;
        g_ForcePartials[1 * groups + groupID.x] = s_Force[0].y;
        g_ForcePartials[2 * groups + groupID.x] = s_Force[0].z;
        g_ForcePartials[3 * groups + groupID.x] = s_Torque[0].x;
        g_ForcePartials[4 * groups + groupID.x] = s_Torque[0].y;
        g_ForcePartials[5 * groups + groupID.x] = s_Torque[0].z;
    }
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