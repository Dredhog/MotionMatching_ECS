#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

layout(location = 4) in ivec4 a_bone_indices;
layout(location = 5) in vec4 a_bone_weights;

uniform mat4 mat_mvp;
uniform vec3 g_bone_colors[20];
uniform mat4 g_bone_matrices[20];

out vec3 frag_color;
out vec3 frag_normal;

void
main()
{
  frag_color = vec3(g_bone_colors[a_bone_indices.x] * a_bone_weights.x +
                    g_bone_colors[a_bone_indices.y] * a_bone_weights.y +
                    g_bone_colors[a_bone_indices.z] * a_bone_weights.z +
                    g_bone_colors[a_bone_indices.w] * a_bone_weights.w);

  frag_normal = a_normal;

  mat4 final_pose_matrix = g_bone_matrices[a_bone_indices.x] * a_bone_weights.x +
                           g_bone_matrices[a_bone_indices.y] * a_bone_weights.y +
                           g_bone_matrices[a_bone_indices.z] * a_bone_weights.z +
                           g_bone_matrices[a_bone_indices.w] * a_bone_weights.w;
  gl_Position = mat_mvp * final_pose_matrix * vec4(a_position, 1.0f);
}
