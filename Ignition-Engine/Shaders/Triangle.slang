struct VSOut
{
    float4 Position : SV_Position;
    float3 Color : COLOR0;
};

[shader("vertex")]
VSOut vertexMain(uint vertexID : SV_VertexID)
{
    const float2 positions[3] =
    {
        float2( 0.0, -0.5),
        float2( 0.5,  0.5),
        float2(-0.5,  0.5),
    };

    const float3 colors[3] =
    {
        float3(1.0, 0.0, 0.0),
        float3(0.0, 1.0, 0.0),
        float3(0.0, 0.0, 1.0),
    };

    VSOut output;
    output.Position = float4(positions[vertexID], 0.0, 1.0);
    output.Color = colors[vertexID];

    return output;
}

[shader("fragment")]
float4 fragmentMain(VSOut input) : SV_Target
{
    return float4(input.Color, 1.0);
}