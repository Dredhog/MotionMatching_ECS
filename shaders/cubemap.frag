#version 330 core
in vec3 frag_texCoords;

uniform samplerCube cubemap;

out vec4 out_color;

void main()
{
  out_color = texture(cubemap, frag_texCoords);
}
