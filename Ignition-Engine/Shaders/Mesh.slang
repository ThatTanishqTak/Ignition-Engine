struct VSIn
{
    float3 Position : POSITION;
    float3 Normal : NORMAL0;
    float3 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

struct VSOut
{
    float4 Position : SV_Position;
    float3 Normal : NORMAL0;
    float3 Color : COLOR0;
    float2 UV : TEXCOORD0;
};

struct PushConstants
{
    float4x4 MVP;
    float4 Tint;
};

[[vk::push_constant]]
PushConstants g_PushConstants;

[[vk::binding(0, 0)]]
Sampler2D g_Texture;

[shader("vertex")]
VSOut vertexMain(VSIn input)
{
    VSOut output;
    output.Position = mul(g_PushConstants.MVP, float4(input.Position, 1.0));
    output.Normal = input.Normal;
    output.Color = input.Color;
    output.UV = input.UV;

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOut input) : SV_Target
{
    return g_Texture.Sample(input.UV) * float4(input.Color, 1.0) * g_PushConstants.Tint;
}