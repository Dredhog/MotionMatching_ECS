#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float cameraNearPlane;
uniform float cameraFarPlane;

uniform sampler2D DepthMap;

float LinearizeDepth(float Depth)
{
    float Z = Depth * 2.0 - 1.0;
    return (2.0 * cameraNearPlane) / (cameraFarPlane + cameraNearPlane - Z * (cameraFarPlane - cameraNearPlane));
}

void main()
{
    float DepthValue = texture(DepthMap, TexCoords).r;
    float Depth = LinearizeDepth(DepthValue);
    FragColor = vec4(vec3(1.0 - Depth), 1.0);
}
