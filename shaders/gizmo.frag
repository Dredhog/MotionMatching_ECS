#version 330 core

in vec3 frag_position;

out vec4 out_color;

void 
main()
{
  out_color = vec4(mix(vec3(1), normalize(frag_position), 20*length(frag_position)), 1);
}
