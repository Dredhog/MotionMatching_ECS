#version 330 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 mat_prev_mvp;
uniform mat4 mat_mvp;
uniform mat4 mat_view;
uniform mat4 mat_model;


out vertex_out
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
  vec4 position0;
  vec4 position1;
}
frag;

void
main()
{
  gl_Position   = mat_mvp * vec4(a_position, 1.0f);
  frag.normal   = mat3(mat_view) * mat3(mat_model) * a_normal;
  frag.position = vec3(mat_view	* mat_model* vec4(a_position, 1.0));
  frag.texCoord = a_texCoord;

  frag.position0 = mat_prev_mvp * vec4(a_position, 1);
  frag.position1 = mat_mvp * vec4(a_position, 1);
}
