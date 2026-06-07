static float2 positions[3] = float2[](
    float2(0.0, -0.5),
    float2(0.5, 0.5),
    float2(-0.5, 0.5)
);

ConstantBuffer<float4x4> viewProj;

struct VertexOutput {
    float4 position : SV_Position;
};

struct Vertex {
    float3 position;
    float3 color;
    float2 texcoord;
}

struct PushConstant {
    Vertex* vertex_buffer;
}

[shader("vertex")]
VertexOutput main(uint vid : SV_VertexID, uniform PushConstant push_contant) {
    VertexOutput output;
    output.position = 
        mul(viewProj, float4(push_contant.vertex_buffer[vid].position, 1.0));
    return output;
}

[shader("fragment")]
float4 main() : SV_Target {
    return float4(1.0, 0.0, 0.0, 1.0);
}