#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

const float Offset = 1.0 / 300.0;

void main()
{
    vec2 Offsets[9] = vec2[](
        vec2(-Offset, Offset),
        vec2(0.0f, Offset),
        vec2(Offset, Offset),
        vec2(-Offset, 0.0f),
        vec2(0.0f, 0.0f),
        vec2(Offset, 0.0f),
        vec2(-Offset, -Offset),
        vec2(0.0f, Offset),
        vec2(Offset, -Offset)
    );

    float Blur[9] = float[](
        1.0 / 16, 2.0 / 16, 1.0 / 16,
        2.0 / 16, 4.0 / 16, 2.0 / 16,
        1.0 / 16, 2.0 / 16, 1.0 / 16
    );

    vec3 SampleTex[9];
    for(int i = 0; i < 9; ++i)
    {
        SampleTex[i] = vec3(texture(ScreenTex, TexCoords.st + Offsets[i]));
    }
    vec3 Col = vec3(0.0);
    for(int i = 0; i < 9; ++i)
    {
        Col += SampleTex[i] * Blur[i];
    }
    FragColor = vec4(Col, 1.0);
}
