#version 330 core

#define COLOR 1
#define TEXTURE 2
#define SPECULAR 4
#define NORMAL 8

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;

uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform vec3 light_position;
uniform vec3 camera_position;
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
  frag.lightPos  = light_position;
  frag.cameraPos = camera_position;
  frag.flags     = flags;

  mat3 normalMatrix = transpose(inverse(mat3(mat_model)));

  if((flags & TEXTURE) != 0)
  {
    frag.texCoord = a_texCoord;
  }
  else
  {
    frag.texCoord = vec2(0.0f);
  }

  if((flags & NORMAL) != 0)
  {
    frag.normal = vec3(0.0f);

    vec3 a_bitangent = cross(a_tangent, a_normal);

    vec3 tangent   = normalize(normalMatrix * a_tangent);
    vec3 bitangent = normalize(normalMatrix * a_bitangent);
    vec3 normal    = normalize(normalMatrix * a_normal);

    mat3 mat_tbn         = transpose(mat3(tangent, bitangent, normal));
    frag.tangentLightPos = mat_tbn * light_position;
    frag.tangentViewPos  = mat_tbn * camera_position;
    frag.tangentFragPos  = mat_tbn * frag.position;
  }
  else
  {
    frag.normal          = normalMatrix * a_normal;
    frag.tangentLightPos = vec3(0.0f);
    frag.tangentViewPos  = vec3(0.0f);
    frag.tangentFragPos  = vec3(0.0f);
  }
}
