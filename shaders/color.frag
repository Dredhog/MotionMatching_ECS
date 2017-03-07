#version 330 core

in vec3 vert_color;

out vec4 out_color;

void main()
{
  out_color = vec4(vert_color, 1.0f);
}
