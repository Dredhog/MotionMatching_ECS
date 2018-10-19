#version 330 core
out vec4 FragColor;

in vertex_out
{
  vec3 position;
  vec3 normal;
  vec2 texCoord;
  vec3 tangentLightPos;
  vec3 tangentViewPos;
  vec3 tangentFragPos;
}
frag;

void
main()
{
  FragColor = vec4(frag.normal, 1.0);
}
