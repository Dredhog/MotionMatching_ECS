#version 330 core

#define DIFFUSE 1
#define SPECULAR 2
#define NORMAL 4

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;

uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform vec3 lightPosition;
uniform vec3 cameraPosition;
uniform int  flags;

out VertexOut
{
  flat int flags;

  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 lightPos;
  vec3 cameraPos;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
}
frag;

void
main()
{
  gl_Position    = mat_mvp * vec4(a_position, 1.0f);
  frag.position  = vec3(mat_model * vec4(a_position, 1.0f));
  frag.lightPos  = lightPosition;
  frag.cameraPos = cameraPosition;
  frag.flags     = flags;

  mat3 normalMatrix = transpose(inverse(mat3(mat_model)));

  frag.texCoord = a_texCoord;

  frag.normal = normalMatrix * a_normal;

  vec3 normal    = normalize(normalMatrix * a_normal);
  vec3 tangent   = normalize(normalMatrix * a_tangent);
  tangent        = normalize(tangent - dot(tangent, normal) * normal);
  vec3 bitangent = cross(normal, tangent);

  mat3 mat_tbn         = transpose(mat3(tangent, bitangent, normal));
  frag.tangentLightPos = mat_tbn * lightPosition;
  frag.tangentViewPos  = mat_tbn * cameraPosition;
  frag.tangentFragPos  = mat_tbn * frag.position;
}
