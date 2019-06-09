#version 330 core

layout(location = 0) in vec3 a_position;

layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;

uniform mat4 g_boneMatrices[20];

uniform mat4 mat_mvp;

void
main()
{
  mat4 finalPoseMatrix = mat4(1.0f);

  if((g_boneMatrices[0] != mat4(0.0f)) &&
     (a_boneWeights.x + a_boneWeights.y + a_boneWeights.z + a_boneWeights.w) > 0.0f)
  {
    finalPoseMatrix = g_boneMatrices[a_boneIndices.x] * a_boneWeights.x +
                      g_boneMatrices[a_boneIndices.y] * a_boneWeights.y +
                      g_boneMatrices[a_boneIndices.z] * a_boneWeights.z +
                      g_boneMatrices[a_boneIndices.w] * a_boneWeights.w;
  }

  gl_Position = mat_mvp * finalPoseMatrix * vec4(a_position, 1.0f);
}
