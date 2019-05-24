#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;

uniform mat4 mat_mvp;
uniform mat4 mat_model;

out vec2 frag_texCoord;
out vec3 frag_normal;
out vec3 frag_position;

void
main()
{
  frag_texCoord = a_texCoord;
  frag_normal   = mat3(transpose(inverse(mat_model))) * a_normal;
  frag_position = vec3(mat_model * vec4(a_position, 1.0f));
  gl_Position   = mat_mvp * vec4(a_position, 1.0f);
}
