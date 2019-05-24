#version 330 core
layout(location = 0) in vec3 a_position;

uniform mat4 mat_projection;
uniform mat4 mat_view;

out vec3 frag_texCoords;

void main()
{
  vec4 position = mat_projection * mat_view * vec4(a_position, 1.0f);
  gl_Position = position.xyww;
  frag_texCoords = a_position;
}
