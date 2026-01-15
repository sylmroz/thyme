struct Camera {
    float4x4 view;
    float4x4 proj;
};

ConstantBuffer<Camera> camera;

struct ModelMatrix {
    float4x4 model;
};

ConstantBuffer<ModelMatrix> modelMatrix;

struct VSInput {
    float3 position;
    float3 color;
    float2 texcoord;
};

struct VSOutput {
    float4 position : SV_Position;
    float3 color;
    float2 texcoord;
};

[shader("vertex")]
VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(camera.proj, mul(camera.view, mul(modelMatrix.model, float4(input.position, 1.0))));
    output.color = input.color;
    output.texcoord = input.texcoord;
    return output;
}

Sampler2D texture;

[shader("fragment")]
float4 main(VSOutput vertexInfo) : SV_Target {
    return texture.Sample(vertexInfo.texcoord);
}
