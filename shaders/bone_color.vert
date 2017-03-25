#version 330 core

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

layout(location = 4) in int[3] a_bone_indices;
layout(location = 5) in float[3] a_bone_weights;

uniform mat4 mat_mvp;
uniform mat3 g_bone_colors[20];

out vec3 frag_color;
out vec3 frag_normal;

void
main()
{
  frag_color = vec3(g_bone_colors[a_bone_indices[0]] * a_bone_weights[0] +
                    g_bone_colors[a_bone_indices[1]] * a_bone_weights[1] +
                    g_bone_colors[a_bone_indices[2]] * a_bone_weights[2]);
  frag_normal = a_normal;
  gl_Position = mat_mvp * vec4(a_position, 1.0f);
}
