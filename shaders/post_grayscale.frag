#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D ScreenTex;

void main()
{
    FragColor = texture(ScreenTex, TexCoords);
    float Average = 0.2126 * FragColor.r + 0.7152 * FragColor.g + 0.0722 * FragColor.b;
    FragColor = vec4(Average, Average, Average, 1.0);
}
