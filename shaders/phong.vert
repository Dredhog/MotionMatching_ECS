#version 330 core

#define DIFFUSE_MAP 1
#define SPECULAR_MAP 2
#define NORMAL_MAP 4
#define SKELETAL 8

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform mat4 g_boneMatrices[20];
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
  mat4 finalPoseMatrix = mat4(1.0f);

  if((flags & SKELETAL) != 0 && (g_boneMatrices[0] != mat4(0.0f)) &&
     (a_boneWeights.x + a_boneWeights.y + a_boneWeights.z + a_boneWeights.w) > 0.99f)
  {
    finalPoseMatrix = g_boneMatrices[a_boneIndices.x] * a_boneWeights.x +
                      g_boneMatrices[a_boneIndices.y] * a_boneWeights.y +
                      g_boneMatrices[a_boneIndices.z] * a_boneWeights.z +
                      g_boneMatrices[a_boneIndices.w] * a_boneWeights.w;
  }

  mat4 modelMatrix = mat_model * finalPoseMatrix;
  mat4 mvpMatrix   = mat_mvp * finalPoseMatrix;

  gl_Position    = mvpMatrix * vec4(a_position, 1.0f);
  frag.position  = vec3(modelMatrix * vec4(a_position, 1.0f));
  frag.lightPos  = lightPosition;
  frag.cameraPos = cameraPosition;
  frag.flags     = flags;

  mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

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
