#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;

uniform vec3 g_position;
uniform vec2 g_dimension;

out vec2 frag_texCoord;

void
main()
{
  frag_texCoord = a_texCoord;

  vec3 final_position =
    2 * vec3((a_position.xy * g_dimension + g_position.xy), g_position.z) - vec3(1, 1, 0);
  gl_Position = vec4(final_position, 1.0f);
}
