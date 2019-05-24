#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;

uniform float u_Time;
uniform float u_HeightScale;
uniform vec3 u_Frequency;
uniform float u_TimeFrequency;
uniform vec3 u_Phase;
uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform vec3 cameraPosition;
uniform vec3 lightPosition;

out VertexOut
{
  vec3 position;
  vec3 normal;
  vec3 tangent;
  vec2 texCoord;
}
frag;

void
main()
{
  frag.position = vec3(mat_model * vec4(a_position, 1.0f));
  frag.normal   = normalize(mat3(mat_model) * a_normal);
  frag.tangent  = normalize(mat3(mat_model) * a_tangent);
  frag.texCoord = a_texCoord;

  float inst_amplitude = u_HeightScale * cos(dot(u_Phase, frag.position) +
                                             length(frag.position)*u_Time*u_TimeFrequency);
  gl_Position   = mat_mvp * vec4(a_position + frag.normal * inst_amplitude, 1.0f);
}
