#version 330 core

uniform int ID;

out vec4 out_color;

void
main()
{
  out_color = vec4(ID, 1.0f);
}
