#version 330 core

uniform vec4 g_color;

out vec4 out_color;

void
main()
{
  out_color = g_color;
}
