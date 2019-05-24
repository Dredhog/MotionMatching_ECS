#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;

layout(location = 4) in ivec4 a_bone_indices;
layout(location = 5) in vec4 a_bone_weights;

uniform mat4 mat_mvp;
uniform mat4 mat_model;
uniform mat4 g_bone_matrices[20];

out vec2 frag_texCoord;
out vec3 frag_normal;
out vec3 frag_position;

void
main()
{
  frag_texCoord = a_texCoord;

  mat4 final_pose_matrix = g_bone_matrices[a_bone_indices.x] * a_bone_weights.x +
                           g_bone_matrices[a_bone_indices.y] * a_bone_weights.y +
                           g_bone_matrices[a_bone_indices.z] * a_bone_weights.z +
                           g_bone_matrices[a_bone_indices.w] * a_bone_weights.w;
  frag_normal = mat3(transpose(inverse(mat_model * final_pose_matrix))) * a_normal;

  frag_position = vec3(mat_model * final_pose_matrix * vec4(a_position, 1.0f));
  gl_Position   = mat_mvp * final_pose_matrix * vec4(a_position, 1.0f);
}
