#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float cameraFarClipPlane;
uniform float OffsetX;
uniform float OffsetY;

uniform sampler2D PositionTex;

void main()
{
    vec2 Offsets[9] = vec2[](
        vec2(-OffsetX, OffsetY),
        vec2(0.0, OffsetY),
        vec2(OffsetX, OffsetY),
        vec2(-OffsetX, 0.0),
        vec2(0.0, 0.0),
        vec2(OffsetX, 0.0),
        vec2(-OffsetX, -OffsetY),
        vec2(0.0, -OffsetY),
        vec2(OffsetX, -OffsetY)
    );

    float Gx[9] = float[](
        1, 0, -1,
        2, 0, -2,
        1, 0, -1
    );

    float Gy[9] = float[](
        1, 2, 1,
        0, 0, 0,
        -1, -2, -1
    );

    float SumX = 0.0;
    float SumY = 0.0;

    for(int i = 0; i < 9; ++i)
    {
        float Sample = length(texture(PositionTex, TexCoords.st + Offsets[i]).xyz) / cameraFarClipPlane;
        SumX += Sample * Gx[i];
        SumY += Sample * Gy[i];
    }

    float Magnitude = sqrt(SumX * SumX + SumY * SumY);

    FragColor = vec4(vec3(1.0 - Magnitude), 1.0);
}
