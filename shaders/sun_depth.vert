#version 330 core

layout(location = 0) in vec3 in_Position;

layout(location = 4) in ivec4 a_boneIndices;
layout(location = 5) in vec4 a_boneWeights;
uniform mat4 g_boneMatrices[70];

uniform mat4 mat_sun_vp;
uniform mat4 mat_model;

void
main()
{
  mat4 temp_mat_model = mat_model;
  if((g_boneMatrices[0][0] != 0) &&
     (a_boneWeights.x + a_boneWeights.y + a_boneWeights.z + a_boneWeights.w) > 0.99f)
  {
    mat4 finalPoseMatrix = g_boneMatrices[a_boneIndices.x] * a_boneWeights.x +
                           g_boneMatrices[a_boneIndices.y] * a_boneWeights.y +
                           g_boneMatrices[a_boneIndices.z] * a_boneWeights.z +
                           g_boneMatrices[a_boneIndices.w] * a_boneWeights.w;
    temp_mat_model *= finalPoseMatrix;
  }
  gl_Position = mat_sun_vp * temp_mat_model * vec4(in_Position, 1.0);
}
