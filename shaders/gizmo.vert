#version 330 core

in vec3 vert_position;

uniform mat4 mat_mvp;
uniform vec3 scale;

uniform float depth;

out vec3 frag_position;

const float gizmo_scale = 0.12;

void
main()
{
  frag_position = vert_position;

  vec3 scaled =
    vert_position * (-depth) * gizmo_scale * vec3(1 / scale.x, 1 / scale.y, 1 / scale.z);
  gl_Position = mat_mvp * vec4(scaled, 1.0f);
}
