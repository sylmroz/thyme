static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

ConstantBuffer<float4x4> viewProj;

struct VertexOutput {
    float4 position : SV_Position;
    float4 color;
};

struct Vertex {
    float4 position;
    float4 color;
    float2 texcoord;
}

struct PushConstant {
    Vertex* vertex_buffer;
}

[shader("vertex")]
VertexOutput main(uint vid : SV_VertexID, uniform PushConstant push_contant) {
    VertexOutput output;
    output.position = 
        mul(viewProj, float4(push_contant.vertex_buffer[vid].position));
    output.color = push_contant.vertex_buffer[vid].color;
    return output;
}

[shader("fragment")]
float4 main(VertexOutput vertexInfo) : SV_Target {
    return float4(vertexInfo.color);
}