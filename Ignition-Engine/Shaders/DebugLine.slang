struct VSIn
{
    float3 Position : POSITION;
    float4 Color : COLOR0;
};

struct VSOut
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

struct PushConstants
{
    float4x4 ViewProjection;
};

[[vk::push_constant]]
PushConstants g_PushConstants;

[shader("vertex")]
VSOut vertexMain(VSIn input)
{
    VSOut output;
    output.Position = mul(g_PushConstants.ViewProjection, float4(input.Position, 1.0));
    output.Color = input.Color;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOut input) : SV_Target
{
    return input.Color;
}