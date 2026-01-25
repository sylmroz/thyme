RWTexture2D<float4> image;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 threadId : SV_DispatchThreadID) {
    uint width, height, mipLevels;
    image.GetDimensions(0, width, height, mipLevels);
    var texCoords = threadId.xy;
    if (texCoords.x < width && texCoords.y < height) {
        var color = float4(0, 0, 0, 0);
        if (texCoords.x != 0 && texCoords.y != 0) {
            color.x = float(texCoords.x) / width;
            color.y = float(texCoords.y) / height;
        }
        image.Store(texCoords, color);
    }
}