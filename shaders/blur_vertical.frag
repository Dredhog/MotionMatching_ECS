#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

const float Offset = 1.0 / 300.0;

void main()
{
    vec2 Offsets[3] = vec2[](
        vec2(0.0f, Offset),
        vec2(0.0f, 0.0f),
        vec2(0.0f, -Offset)
    );

    float Blur[3] = float[](
        1.0 / 4.0, 2.0 / 4.0, 1.0 / 4.0
    );

    vec3 SampleTex[3];
    for(int i = 0; i < 3; ++i)
    {
        SampleTex[i] = vec3(texture(ScreenTex, TexCoords.st + Offsets[i]));
    }
    vec3 Col = vec3(0.0);
    for(int i = 0; i < 3; ++i)
    {
        Col += SampleTex[i] * Blur[i];
    }
    FragColor = vec4(Col, 1.0);
}
