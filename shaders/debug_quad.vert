#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform vec4 g_color;
uniform vec3 g_position;
uniform vec2 g_dimension;

out vec4 frag_color;

void
main()
{
  frag_color = g_color;

  vec3 final_position =
    vec3(a_position.xy * g_dimension - vec2(1, 1) + g_position.xy, g_position.z);
  gl_Position = vec4(final_position, 1.0f);
}
