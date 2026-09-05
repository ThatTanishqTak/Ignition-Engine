struct UIPrimitive
{
    float4 Bounds;  // xy = centre in surface pixels, zw = half extent
    float4 Radius;  // corner radii, clockwise from the top left
    float2 Edge;    // x = ring half width (0 fills), y = the smoothstep half width - AA at 0.5, a soft shadow above it
    uint Mode;
    uint Texture;   // texture table slot; 0 is the permanent 1x1 white
};

static const uint ModeSolid = 0;
static const uint ModeRoundedRect = 1;
static const uint ModeTexture = 2;
static const uint ModeGlyphCoverage = 3;
static const uint ModeGlyphSDF = 4;

struct VSIn
{
    float2 Position : POSITION;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
    uint Primitive : TEXCOORD1;
};

struct VSOut
{
    float4 Position : SV_Position;
    float2 UV : TEXCOORD0;
    float4 Color : COLOR0;
    float2 Local : TEXCOORD1; // position relative to the primitive centre, which is what the SDF is evaluated in
    nointerpolation uint Primitive : TEXCOORD2;
};

struct PushConstants
{
    float4x4 Projection; // ortho for the target surface, with the pass's viewport flip already baked into one sign
};

[[vk::push_constant]]
PushConstants g_PushConstants;

// Set 0 is per frame in flight and grows with the frame's primitive count; set 1 is the renderer-wide texture table
[[vk::binding(0, 0)]]
StructuredBuffer<UIPrimitive> g_Primitives;

[[vk::binding(0, 1)]]
Sampler2D g_Textures[];

// §6.5's rounded box. y runs down in surface space, so each side's pair reads (top, bottom)
float SignedDistanceRoundedBox(float2 point, float2 halfExtent, float4 radius)
{
    const float2 side = point.x > 0.0 ? float2(radius.y, radius.z) : float2(radius.x, radius.w);
    const float corner = point.y > 0.0 ? side.y : side.x;
    const float2 q = abs(point) - halfExtent + corner;

    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - corner;
}

[shader("vertex")]
VSOut vertexMain(VSIn input)
{
    const UIPrimitive primitive = g_Primitives[input.Primitive];

    VSOut output;
    output.Position = mul(g_PushConstants.Projection, float4(input.Position, 0.0, 1.0));
    output.UV = input.UV;
    output.Color = input.Color;
    output.Local = input.Position - primitive.Bounds.xy;
    output.Primitive = input.Primitive;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOut input) : SV_Target
{
    const UIPrimitive primitive = g_Primitives[input.Primitive];

    float4 color = input.Color;

    switch (primitive.Mode)
    {
        case ModeRoundedRect:
        {
            float distance = SignedDistanceRoundedBox(input.Local, primitive.Bounds.zw, primitive.Radius);

            // A border and a soft shadow are this same quad - only Edge differs
            if (primitive.Edge.x > 0.0)
            {
                distance = abs(distance) - primitive.Edge.x;
            }

            const float softness = max(primitive.Edge.y, 1e-4);

            color.a *= 1.0 - smoothstep(-softness, softness, distance);

            break;
        }
        case ModeTexture:
        {
            color *= g_Textures[NonUniformResourceIndex(primitive.Texture)].Sample(input.UV);

            break;
        }
        case ModeGlyphCoverage:
        case ModeGlyphSDF:
        {
            // TODO(Phase 2): coverage is Color * pow(atlas.Sample(UV).r, CoverageGamma) with gamma ~ 1/1.4; the SDF
            // branch is D9's conditional path and stays unbuilt until Q3's condition fires
            break;
        }
        default:
        {
            // ModeSolid - the tessellator's fringe already carries the coverage in the vertex alpha
            break;
        }
    }

    // Premultiplied: the pipeline blends ONE / ONE_MINUS_SRC_ALPHA
    return float4(color.rgb * color.a, color.a);
}