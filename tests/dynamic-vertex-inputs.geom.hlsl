struct Vertex
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
};

[maxvertexcount(3)]
void main(triangle Vertex input[3], inout TriangleStream<Vertex> output)
{
    for (uint index = 0; index < 3; ++index) output.Append(input[index]);
    output.RestartStrip();
}
