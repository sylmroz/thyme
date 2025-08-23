#version 450

layout (binding = 0) uniform Camera {
    mat4 view;
    mat4 proj;
} camera;

layout (binding = 1) uniform ModelMatix {
    mat4 model;
} modelMatrix;

layout (location = 0) in vec3 inPosition;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inCTexCoord;

layout (location = 0) out vec3 color;
layout (location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = camera.proj * camera.view * modelMatrix.model * vec4(inPosition, 1.0);
    color = inColor;
    fragTexCoord = inCTexCoord;
}