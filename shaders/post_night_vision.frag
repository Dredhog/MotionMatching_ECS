#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

void main()
{
    float Length = texture(ScreenTex, TexCoords).g;
    FragColor = vec4(0.0, Length, 0.0, 1.0);
}
