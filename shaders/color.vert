#version 330 core

layout(location = 0) in vec3 a_position;

uniform mat4 mat_mvp;

void
main()
{
  gl_Position = mat_mvp * vec4(a_position, 1.0f);
}
