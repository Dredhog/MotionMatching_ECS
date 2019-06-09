#version 330 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D EdgeTex;
uniform sampler2D ScreenTex;

void main()
{
    vec4 EdgeColor = texture(EdgeTex, TexCoords);
    vec4 ScreenColor = texture(ScreenTex, TexCoords);
    FragColor = EdgeColor * ScreenColor;
}
