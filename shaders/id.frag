#version 330 core

uniform vec4 g_id;

out vec4 out_color;

void
main()
{
  out_color = g_id;
}
