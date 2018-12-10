#version 330 core

#define PI 3.14159265359

out vec4 FragColor;

in vec2 TexCoords;

// uniform float fogFarDistance;
uniform float cameraNearPlane;
uniform float cameraFarPlane;
uniform float density;
uniform float gradient;
uniform vec3 fogColor;

uniform sampler2D DepthTex;
uniform sampler2D ScreenTex;

void
main()
{
    float Distance = 1.0 - texture(DepthTex, TexCoords).r;
    Distance = mix(cameraNearPlane, cameraFarPlane, Distance);

    float Visibility = exp(-pow((Distance * density), gradient));
    Visibility = clamp(Visibility, 0.0, 1.0);

    FragColor = texture(ScreenTex, TexCoords);
    FragColor = mix(vec4(fogColor, 1.0), FragColor, Visibility);
}
