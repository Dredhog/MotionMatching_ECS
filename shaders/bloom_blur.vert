#version 330 core
layout (location = 0) in vec2 in_Position;
layout (location = 1) in vec2 in_TexCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(in_Position.x, in_Position.y, 0.0, 1.0);
    TexCoords = in_TexCoords;
}
