#version 330 core

layout(location = 0) in vec3 a_position;

uniform mat4 mat_mvp;
uniform mat4 g_bone_matrices[20];

void
main()
{
  mat4 final_pose_matrix = g_bone_matrices[a_bone_indices.x] * a_bone_weights.x +
                           g_bone_matrices[a_bone_indices.y] * a_bone_weights.y +
                           g_bone_matrices[a_bone_indices.z] * a_bone_weights.z +
                           g_bone_matrices[a_bone_indices.w] * a_bone_weights.w;
  gl_Position = mat_mvp * final_pose_matrix * vec4(a_position, 1.0f);
}
