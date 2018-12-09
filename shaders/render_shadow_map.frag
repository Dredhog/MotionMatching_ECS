#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float sunNearPlane;
uniform float sunFarPlane;

uniform sampler2D DepthMap;

float LinearizeDepth(float Depth)
{
    float Z = Depth * 2.0 - 1.0;
    return (2.0 * sunNearPlane * sunFarPlane) / (sunFarPlane + sunNearPlane - Z * (sunFarPlane - sunNearPlane));
}

void main()
{
    float DepthValue = texture(DepthMap, TexCoords).r;
    float Depth = LinearizeDepth(DepthValue) / sunFarPlane;
    FragColor = vec4(vec3(DepthValue), 1.0);
}
