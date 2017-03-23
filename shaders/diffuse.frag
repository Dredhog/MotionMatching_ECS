#version 330 core

in vec3 vert_color;
in vec3 frag_normal;

out vec4 out_color;

void main()
{
 out_color = vec4(vec3(dot(normalize(frag_normal), normalize(vec3(0.25, 1, 0.25)))), 1.0f);
}
