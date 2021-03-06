#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform mat4 mat_sun_vp;
uniform mat4 g_boneMatrices[20];
uniform vec3 lightPosition;
uniform vec3 sunDirection;
uniform vec3 cameraPosition;

out VertexOut
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 lightPos;
  vec3 sunDir;
  vec3 cameraPos;
  vec4 sunFragPos;
}
frag;

void
main()
{
  mat4 modelMatrix = mat_model;
  mat4 mvpMatrix   = mat_mvp;

  if((g_boneMatrices[0] != mat4(0.0f)) &&
     (a_boneWeights.x + a_boneWeights.y + a_boneWeights.z + a_boneWeights.w) > 0.99f)
  {
    mat4 finalPoseMatrix = g_boneMatrices[a_boneIndices.x] * a_boneWeights.x +
                           g_boneMatrices[a_boneIndices.y] * a_boneWeights.y +
                           g_boneMatrices[a_boneIndices.z] * a_boneWeights.z +
                           g_boneMatrices[a_boneIndices.w] * a_boneWeights.w;
    modelMatrix *= finalPoseMatrix;
    mvpMatrix *= finalPoseMatrix;
  }

  gl_Position    = mvpMatrix * vec4(a_position, 1.0f);
  frag.position  = vec3(modelMatrix * vec4(a_position, 1.0f));
  frag.lightPos  = lightPosition;
  frag.sunDir    = sunDirection;
  frag.cameraPos = cameraPosition;

  mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

  frag.texCoord = a_texCoord;

  frag.normal = normalMatrix * a_normal;

  frag.sunFragPos = mat_sun_vp * vec4(frag.position, 1.0f);
}
