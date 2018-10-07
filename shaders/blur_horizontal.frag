#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float Offset;
uniform float Kernel[5];

uniform sampler2D ScreenTex;

void main()
{
    vec2 Offsets[5] = vec2[](
        vec2(-2 * Offset, 0.0f),
        vec2(-Offset, 0.0f),
        vec2(0.0f, 0.0f),
        vec2(Offset, 0.0f),
        vec2(2 * Offset, 0.0f)
    );

    vec3 SampleTex[5];
    for(int i = 0; i < 5; ++i)
    {
        SampleTex[i] = vec3(texture(ScreenTex, TexCoords.st + Offsets[i]));
    }
    vec3 Col = vec3(0.0);
    for(int i = 0; i < 5; ++i)
    {
        Col += SampleTex[i] * Kernel[i];
    }
    FragColor = vec4(Col, 1.0);
}
