#version 330 core

#define PI 3.14159265359

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

float
Amplification(float position)
{
    position = 2.0f * position - 1.0f;
    return 1.0f + cos(position * PI) * 0.5f;
}

void main()
{
    float Length = 0.5f * Amplification(TexCoords.x) * Amplification(TexCoords.y) * texture(ScreenTex, TexCoords).g;
    FragColor = vec4(0.0, Length, 0.0, 1.0);
}
