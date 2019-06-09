#version 330 core

#define PI 3.14159265359

out vec4 FragColor;

in vec2 TexCoords;

uniform float cameraNearPlane;
uniform float cameraFarPlane;
uniform float density;
uniform float gradient;
uniform float fogColor;

uniform sampler2D PositionTex;
uniform sampler2D ScreenTex;

void
main()
{
    float Distance = length(texture(PositionTex, TexCoords).xyz);

    float Visibility = exp(-pow((Distance * density), gradient));
    Visibility = clamp(Visibility, 0.0, 1.0);

    FragColor = texture(ScreenTex, TexCoords);
    FragColor = mix(vec4(vec3(fogColor), 1.0), FragColor, Visibility);
}
