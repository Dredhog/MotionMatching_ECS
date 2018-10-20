#version 330 core

#define KERNEL_SIZE 11

out vec4 FragColor;

in vec2 TexCoords;

uniform float Offset;
uniform float Kernel[KERNEL_SIZE];

uniform sampler2D ScreenTex;

void main()
{
    vec2 Offsets[KERNEL_SIZE] = vec2[](
        vec2(0.0f, -5 * Offset),
        vec2(0.0f, -4 * Offset),
        vec2(0.0f, -3 * Offset),
        vec2(0.0f, -2 * Offset),
        vec2(0.0f, -Offset),
        vec2(0.0f, 0.0f),
        vec2(0.0f, Offset),
        vec2(0.0f, 2 * Offset),
        vec2(0.0f, 3 * Offset),
        vec2(0.0f, 4 * Offset),
        vec2(0.0f, 5 * Offset)
    );

    vec3 SampleTex[KERNEL_SIZE];
    for(int i = 0; i < KERNEL_SIZE; ++i)
    {
        SampleTex[i] = vec3(texture(ScreenTex, TexCoords.st + Offsets[i]));
    }
    vec3 Col = vec3(0.0);
    for(int i = 0; i < KERNEL_SIZE; ++i)
    {
        Col += SampleTex[i] * Kernel[i];
    }
    FragColor = vec4(Col, 1.0);
}
