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
        vec2(-5 * Offset, 0.0f),
        vec2(-4 * Offset, 0.0f),
        vec2(-3 * Offset, 0.0f),
        vec2(-2 * Offset, 0.0f),
        vec2(-Offset, 0.0f),
        vec2(0.0f, 0.0f),
        vec2(Offset, 0.0f),
        vec2(2 * Offset, 0.0f),
        vec2(3 * Offset, 0.0f),
        vec2(4 * Offset, 0.0f),
        vec2(5 * Offset, 0.0f)
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
