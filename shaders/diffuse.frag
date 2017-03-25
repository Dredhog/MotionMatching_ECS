#version 330 core

in vec3 frag_color;
in vec3 frag_normal;

out vec4 out_color;

vec3 light_dir = normalize(vec3(0.25, 1, 0.25));

void
main()
{
  vec3 unit_normal = normalize(frag_normal);
  out_color   = vec4((vec3(dot(unit_normal , light_dir))), 1.0f);
}
