struct Input
{
    [[vk::location(0)]] float3 Position : ATTRIBUTE0;
    [[vk::location(1)]] float4 Color : ATTRIBUTE1;
};

struct Output
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

Output main(Input input)
{
    Output output;
    output.Position = float4(input.Position, 1.0);
    output.Color = input.Color;
    return output;
}
