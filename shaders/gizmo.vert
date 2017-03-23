#version 330 core

in vec3 vert_position;

uniform mat4  mat_mvp;
uniform float depth;

out vec3 frag_position;

void
main()
{
  frag_position = vert_position;

  vec3 scaled = vert_position * (-depth);
  gl_Position = mat_mvp * vec4(scaled, 1.0f);
}
