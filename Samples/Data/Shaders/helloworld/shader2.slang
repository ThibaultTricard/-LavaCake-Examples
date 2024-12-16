struct VertexIn {
    float4 position : POSITION;
    float3 color : COLOR;
};

struct VertexOut {
    float4 position : SV_Position;
    float3 color : CoarseVertex;
};

[shader("vertex")]
VertexOut vMain(
    VertexIn vertex)
{
    VertexOut output;

    output.position = vertex.position;
    output.color = vertex.color;
    return output;
}

struct Fragment {
    float4 color;
};

[shader("fragment")]
Fragment fMain(float3 color: CoarseVertex) {
    return { float4(color,1.0) };
}
  