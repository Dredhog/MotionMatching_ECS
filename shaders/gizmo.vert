#version 330 core

in vec3 vert_position;

uniform mat4 mat_mvp;

uniform vec3  scale;
uniform float depth;
uniform float gizmo_scale_factor;

out vec3 frag_position;

void
main()
{
  frag_position = vert_position;

  // Note(Lukas) division by scale vec3 to cancel out potentially nonuniform model scale
  vec3 scaled = vert_position * gizmo_scale_factor * (-depth / scale);
  gl_Position = mat_mvp * vec4(scaled, 1.0f);
}
