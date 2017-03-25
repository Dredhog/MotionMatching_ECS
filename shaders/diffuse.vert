#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

uniform mat4 mat_mvp;

out vec3 frag_color;
out vec3 frag_normal;

void
main()
{
  frag_color    = vec3(1.0);
  frag_normal   = normal;
  gl_Position   = mat_mvp * vec4(position, 1.0f);
}
