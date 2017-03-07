#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

uniform mat4 mat_mvp;

out vec3 vert_color;

void
main()
{
  vert_color  = color;
  gl_Position = mat_mvp * vec4(position, 1.0f);
}
